// DtBlkFix plugin: the engine side of EngineBridge.h

// as every engine source does: windows.h, the plugin base class, VSTGUI, using namespace std
#include <StdAfx.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdint.h>

#include "DtBlkFx.hpp"
#include "EngineBridge.h"
#include "Gui.h"

// from DtBlkFxMain.cpp
extern CharArray<4096> g_plugin_path;
extern void* hInstance;
bool DtBlkFixGlobalInit();
bool DtBlkFixLoadImages(const char* skin_dir, float scale, GuiImages* dst, ostream* err);

namespace dtblkfix {

//-------------------------------------------------------------------------------------------------
static HMODULE ThisModule()
{
  // the module this code is in (StdAfx.h targets Windows 2000, before GetModuleHandleEx)
  MEMORY_BASIC_INFORMATION mbi;
  if (!VirtualQuery((void*)&ThisModule, &mbi, sizeof(mbi)))
    return NULL;
  return (HMODULE)mbi.AllocationBase;
}

static bool Exists(const std::string& path)
{
  return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

//-------------------------------------------------------------------------------------------------
// The single-file plugin carries its "dtblkfx" folder and the licenses as a resource, packed by
// tools/pack_resources.py: "DTBXPAK1", then for each file a u32 name length, the name, a u32 size
// and the data, ending with a name length of 0. It's unpacked once, to
// %LOCALAPPDATA%\DtBlkFix\<version>\, with a ".pack" file holding the pack's fingerprint.

static uint32_t ReadU32(const unsigned char* p)
{
  return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

static std::string Fingerprint(const unsigned char* p, size_t size)
{
  uint64_t h = 14695981039346656037ull; // FNV-1a
  for (size_t i = 0; i < size; i++)
    h = (h ^ p[i]) * 1099511628211ull;
  char s[17];
  sprintf(s, "%016llx", (unsigned long long)h);
  return s;
}

static std::string ReadText(const std::string& path)
{
  std::ifstream f(path.c_str());
  std::string s;
  std::getline(f, s);
  return s;
}

// makes every folder in dir (which ends with '\')
static bool MakeDirs(const std::string& dir)
{
  for (size_t i = dir.find('\\', 3); i != std::string::npos; i = dir.find('\\', i + 1))
    CreateDirectoryA(dir.substr(0, i).c_str(), NULL);
  return Exists(dir);
}

static void DeleteTree(const std::string& dir)
{
  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA((dir + "*").c_str(), &fd);
  if (h != INVALID_HANDLE_VALUE) {
    do {
      std::string name = fd.cFileName;
      if (name == "." || name == "..")
        continue;
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        DeleteTree(dir + name + "\\");
      else
        DeleteFileA((dir + name).c_str());
    } while (FindNextFileA(h, &fd));
    FindClose(h);
  }
  RemoveDirectoryA(dir.c_str());
}

static bool WriteAll(const std::string& path, const void* data, size_t size)
{
  HANDLE h = CreateFileA(
      path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE)
    return false;
  DWORD written = 0;
  bool ok = WriteFile(h, data, (DWORD)size, &written, NULL) && written == size;
  CloseHandle(h);
  return ok;
}

// unpacks into dir (which ends with '\'); false if the pack is damaged or a write failed
static bool Unpack(const unsigned char* p, size_t size, const std::string& dir)
{
  const unsigned char* end = p + size;
  if (size < 8 || memcmp(p, "DTBXPAK1", 8) != 0)
    return false;
  p += 8;
  for (;;) {
    if (end - p < 4)
      return false;
    uint32_t name_len = ReadU32(p);
    p += 4;
    if (name_len == 0)
      return true;
    if ((size_t)(end - p) < (size_t)name_len + 4)
      return false;
    std::string name((const char*)p, name_len);
    p += name_len;
    uint32_t len = ReadU32(p);
    p += 4;
    if ((size_t)(end - p) < len || name.find("..") != std::string::npos)
      return false;
    std::string path = dir + name;
    if (!MakeDirs(path.substr(0, path.find_last_of('\\') + 1)) || !WriteAll(path, p, len))
      return false;
    p += len;
  }
}

// the folder holding the unpacked "dtblkfx\", or "" if there's no pack or it can't be unpacked
static std::string UnpackedDir(HMODULE module)
{
  HRSRC res = FindResourceA(module, "DTBLKFIX_RESOURCES", MAKEINTRESOURCEA(10) /*RT_RCDATA*/);
  HGLOBAL loaded = res ? LoadResource(module, res) : NULL;
  const unsigned char* pack = loaded ? (const unsigned char*)LockResource(loaded) : NULL;
  if (!pack)
    return "";
  size_t size = SizeofResource(module, res);

  char appdata[MAX_PATH * 2];
  DWORD n = GetEnvironmentVariableA("LOCALAPPDATA", appdata, sizeof(appdata));
  if (n == 0 || n >= sizeof(appdata))
    return "";
  std::string base = std::string(appdata, n) + "\\DtBlkFix\\";
  std::string dir = base + DTBLKFIX_VERSION "\\";
  std::string id = Fingerprint(pack, size);
  if (ReadText(dir + ".pack") == id)
    return dir;

  // unpack into a folder of its own, then move it into place, so that a host loading the plugin at
  // the same time never sees half of it
  std::ostringstream tmp;
  tmp << base << DTBLKFIX_VERSION << ".unpacking-" << GetCurrentProcessId() << "\\";
  DeleteTree(tmp.str());
  if (!MakeDirs(tmp.str()) || !Unpack(pack, size, tmp.str()) ||
      !WriteAll(tmp.str() + ".pack", id.data(), id.size())) {
    DeleteTree(tmp.str());
    return "";
  }
  if (ReadText(dir + ".pack") != id) {
    DeleteTree(dir); // an older or half-written unpack of this version
    if (MoveFileA(tmp.str().substr(0, tmp.str().size() - 1).c_str(),
                  dir.substr(0, dir.size() - 1).c_str()))
      return dir;
  }
  if (ReadText(dir + ".pack") == id) { // another host got there first
    DeleteTree(tmp.str());
    return dir;
  }
  return tmp.str(); // the old folder is in use: run from this one
}

// folder holding "dtblkfx\": next to the plugin, in the VST3 bundle's Resources folder, or (for the
// single-file plugin) unpacked from the plugin
static std::string PluginDir()
{
  char path[4096] = {0};
  GetModuleFileNameA(ThisModule(), path, sizeof(path) - 1);
  std::string dir = path;
  dir = dir.substr(0, dir.find_last_of("\\/") + 1);
  std::string resources = dir + "..\\Resources\\";
  if (Exists(dir + "dtblkfx"))
    return dir;
  if (Exists(resources + "dtblkfx"))
    return resources;
  std::string unpacked = UnpackedDir(ThisModule());
  return unpacked.empty() ? dir : unpacked;
}

static std::string SkinDir(int idx)
{
  std::vector<std::string> names = Engine::skinNames();
  std::string dir = g_plugin_path.toString() + "dtblkfx";
  if (idx > 0 && idx < (int)names.size())
    dir += "\\skins\\" + names[idx];
  return dir;
}

//-------------------------------------------------------------------------------------------------
static CriticalSectionWrapper g_init_protection;

bool Engine::init()
{
  static bool done = false, ok = false;
  ScopeCriticalSection scs(g_init_protection);
  if (done)
    return ok;
  done = true;

  hInstance = ThisModule();
  Clear(g_plugin_path);
  g_plugin_path << PluginDir().c_str();
  try {
    ok = DtBlkFixGlobalInit();
  }
  catch (...) {
    ok = false;
  }
  return ok;
}

std::vector<std::string> Engine::skinNames()
{
  std::vector<std::string> names;
  WIN32_FIND_DATAA fd;
  std::string pattern = g_plugin_path.toString() + "dtblkfx\\skins\\*";
  HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
  if (h != INVALID_HANDLE_VALUE) {
    do {
      if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] != '.')
        names.push_back(fd.cFileName);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
  }
  std::sort(names.begin(), names.end());
  names.insert(names.begin(), "Original");
  return names;
}

std::vector<int> Engine::scales()
{
  int s[] = {75, 100, 125, 150, 200};
  return std::vector<int>(s, s + sizeof(s) / sizeof(s[0]));
}

//-------------------------------------------------------------------------------------------------
struct Engine::Impl
    : public AudioEffectHost
    , public GuiViewListener {
  EngineListener* listener;
  DtBlkFx* fx;
  VstTimeInfo ti;
  bool have_ti;

  virtual VstTimeInfo* getTimeInfo(VstInt32 /*filter*/) { return have_ti ? &ti : NULL; }
  virtual void ioChanged() { listener->latencyChanged(); }
  virtual void parameterAutomated(VstInt32 index, float value)
  {
    listener->paramChangedByEditor(index, value);
  }
  virtual void beginEdit(VstInt32 index) { listener->beginEdit(index); }
  virtual void endEdit(VstInt32 index) { listener->endEdit(index); }
  virtual void guiViewChange(int scale_pct, int skin_idx, int style)
  {
    listener->viewChangeRequested(scale_pct, skin_idx, style);
  }
  virtual void guiProgramChange(int program)
  {
    if (program < 0 || program >= fx->getNumPrograms())
      return;
    fx->setProgram(program);
    listener->programChangedByEditor();
  }
  virtual void guiPresetLoaded() { listener->programChangedByEditor(); }
};

Engine::Engine(EngineListener* listener)
    : impl(new Impl)
{
  impl->listener = listener;
  impl->have_ti = false;
  memset(&impl->ti, 0, sizeof(impl->ti));
  impl->fx = new DtBlkFx(NULL);
  impl->fx->setHost(impl);

  // the editor shows only the effects in use (+ and x), which needs the listener to resize
  if (impl->fx->gui())
    impl->fx->gui()->_view_listener = impl;
}

Engine::~Engine()
{
  delete impl->fx;
  delete impl;
}

//-------------------------------------------------------------------------------------------------
int Engine::numParams() const
{
  return impl->fx->getNumParams();
}

float Engine::getParam(int index)
{
  return impl->fx->getParameter(index);
}

void Engine::setParam(int index, float value)
{
  impl->fx->setParameter(index, value);
}

std::string Engine::paramName(int index)
{
  char text[256] = {0};
  impl->fx->getParameterName(index, text);
  return text;
}

std::string Engine::paramDisplay(int index)
{
  char text[256] = {0};
  impl->fx->getParameterDisplay(index, text);
  return text;
}

//-------------------------------------------------------------------------------------------------
int Engine::numPrograms() const
{
  return impl->fx->getNumPrograms();
}

int Engine::getProgram()
{
  return impl->fx->getProgram();
}

void Engine::setProgram(int index)
{
  impl->fx->setProgram(index);
}

std::string Engine::programName(int index)
{
  char text[256] = {0};
  impl->fx->getProgramNameIndexed(0, index, text);
  return text;
}

void Engine::setProgramName(int index, const std::string& name)
{
  if (index >= 0 && index < (int)impl->fx->_program.size())
    impl->fx->_program[index].setName(name.c_str());
}

std::vector<uint8_t> Engine::getChunk(bool is_preset)
{
  void* data = NULL;
  VstInt32 n = impl->fx->getChunk(&data, is_preset);
  if (!data || n <= 0)
    return std::vector<uint8_t>();
  return std::vector<uint8_t>((uint8_t*)data, (uint8_t*)data + n);
}

bool Engine::setChunk(const void* data, size_t size, bool is_preset)
{
  return impl->fx->setChunk(const_cast<void*>(data), (VstInt32)size, is_preset) != 0;
}

//-------------------------------------------------------------------------------------------------
void Engine::prepare(double sample_rate, int max_block)
{
  impl->fx->setSampleRate((float)sample_rate);
  impl->fx->setBlockSize(max_block);
}

void Engine::suspend()
{
  impl->fx->suspend();
}

void Engine::resume()
{
  impl->fx->resume();
}

void Engine::process(const float* const* in, float* const* out, int n, const SongPos* pos)
{
  impl->have_ti = pos != NULL;
  if (pos) {
    VstTimeInfo& ti = impl->ti;
    ti.sampleRate = impl->fx->getSampleRate();
    ti.tempo = pos->tempo;
    ti.ppqPos = pos->ppq;
    ti.timeSigNumerator = pos->sig_num;
    ti.timeSigDenominator = pos->sig_den;
    ti.flags = (pos->tempo_valid ? kVstTempoValid : 0) | (pos->ppq_valid ? kVstPpqPosValid : 0) |
               (pos->playing ? kVstTransportPlaying : 0) | kVstTimeSigValid;
  }
  impl->fx->processReplacing(const_cast<float**>(in), const_cast<float**>(out), n);
}

int Engine::latency() const
{
  return impl->fx->getInitialDelay();
}

//-------------------------------------------------------------------------------------------------
bool Engine::hasEditor() const
{
  return impl->fx->gui() != NULL;
}

void Engine::editorSize(int scale_pct, int& width, int& height) const
{
  ERect* r = NULL;
  width = height = 0;
  if (!impl->fx->gui() || !impl->fx->gui()->getRect(&r) || !r)
    return;
  width = (int)(r->right * scale_pct / 100.0f + 0.5f);
  height = (int)(r->bottom * scale_pct / 100.0f + 0.5f);
}

static LRESULT CALLBACK EditorWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  return DefWindowProcA(hwnd, msg, wp, lp);
}

void* Engine::createEditorWindow(int width, int height)
{
  static const char* class_name = "DtBlkFixEditorHost";
  WNDCLASSA wc;
  if (!GetClassInfoA((HINSTANCE)hInstance, class_name, &wc)) {
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = EditorWindowProc;
    wc.hInstance = (HINSTANCE)hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = class_name;
    RegisterClassA(&wc);
  }
  // a hidden popup, the wrapper makes it a child of its own window
  return CreateWindowExA(0,
                         class_name,
                         "DtBlkFix",
                         WS_POPUP | WS_CLIPCHILDREN,
                         0,
                         0,
                         width,
                         height,
                         NULL,
                         NULL,
                         (HINSTANCE)hInstance,
                         NULL);
}

bool Engine::openEditor(void* window, int scale_pct, int skin_idx, int style)
{
  Gui* gui = impl->fx->gui();
  if (!gui || !window)
    return false;

  float scale = scale_pct / 100.0f;
  std::vector<std::string> skins = skinNames();
  if (skin_idx < 0 || skin_idx >= (int)skins.size())
    skin_idx = 0;

  // the skin's images at this scale (the original skin if it can't be loaded)
  GuiImages images;
  std::ostringstream err;
  if (!DtBlkFixLoadImages(SkinDir(skin_idx).c_str(), scale, &images, &err)) {
    skin_idx = 0;
    if (!DtBlkFixLoadImages(SkinDir(0).c_str(), scale, &images, &err))
      return false;
  }
  gui->setView(scale, images, skins, skin_idx, style, scales(), impl);
  return gui->open(window);
}

void Engine::setEditorWindowCorner(void* window, int width, int height, int corner_w, int corner_h)
{
  if (!window)
    return;
  HRGN rgn = CreateRectRgn(0, 0, width, height);
  if (corner_w > 0 && corner_h > 0) {
    HRGN c = CreateRectRgn(width - corner_w, height - corner_h, width, height);
    CombineRgn(rgn, rgn, c, RGN_DIFF);
    DeleteObject(c);
  }
  // the window owns the region from here
  if (!SetWindowRgn((HWND)window, rgn, TRUE))
    DeleteObject(rgn);
}

bool Engine::editorColours(uint32_t& background, uint32_t& label) const
{
  Gui* gui = impl->fx->gui();
  if (!gui || !gui->_ok || !gui->_images.glob_bg)
    return false;
  // the plain background at the bottom left of the global controls' image (the alpha byte is
  // dropped), and the label colour that suits it
  background = (uint32_t)(PtrRGBA(gui->_images.glob_bg->data())._get() & 0xffffff);
  const CColor& c = gui->_palette.label;
  label = ((uint32_t)c.red << 16) | ((uint32_t)c.green << 8) | (uint32_t)c.blue;
  return true;
}

void Engine::closeEditor()
{
  if (impl->fx->gui())
    impl->fx->gui()->close();
}

void Engine::idleEditor()
{
  if (impl->fx->gui())
    impl->fx->gui()->idle();
}

} // namespace dtblkfix
