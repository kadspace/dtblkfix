// DtBlkFix plugin: the engine side of EngineBridge.h

// as every engine source does: windows.h, the plugin base class, VSTGUI, using namespace std
#include <StdAfx.h>

#include <algorithm>
#include <sstream>

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

// folder holding "dtblkfx\": next to the plugin, or in the VST3 bundle's Resources folder
static std::string PluginDir()
{
  char path[4096] = {0};
  GetModuleFileNameA(ThisModule(), path, sizeof(path) - 1);
  std::string dir = path;
  dir = dir.substr(0, dir.find_last_of("\\/") + 1);
  std::string resources = dir + "..\\Resources\\";
  if (!Exists(dir + "dtblkfx") && Exists(resources + "dtblkfx"))
    return resources;
  return dir;
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
  virtual void guiViewChange(int scale_pct, int skin_idx)
  {
    listener->viewChangeRequested(scale_pct, skin_idx);
  }
};

Engine::Engine(EngineListener* listener)
    : impl(new Impl)
{
  impl->listener = listener;
  impl->have_ti = false;
  memset(&impl->ti, 0, sizeof(impl->ti));
  impl->fx = new DtBlkFx(NULL);
  impl->fx->setHost(impl);
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

bool Engine::openEditor(void* window, int scale_pct, int skin_idx)
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
  gui->setView(scale, images, skins, skin_idx, scales(), impl);
  return gui->open(window);
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
