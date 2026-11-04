// DtBlkFix tests: plugin state and editor through the wrapper's engine side (EngineBridge).
//
// dtblkfix_bridge_test   (needs the "dtblkfx" folder next to it, as the plugin does)
//
// 1. State round trip: settings, programs, renamed program, editor size, skin & style come back,
//    the state is the same bytes again and renders the same audio.
// 2. A bare DtBlkFx chunk (as the VST2 version saves) loads with the default size, skin & style.
// 3. The editor opens at every size in the menu, a size dragged to and the largest, in every skin
//    & style, at the right size: one row per effect in use plus the "+" row.
// 4. In both styles, clicking "+" adds a do-nothing filter after the last effect, clicking a
//    row's "x" removes that effect and moves the ones below up, the header's next & previous
//    buttons load the next & previous presets, and its gear opens & closes the settings page.
// 5. Saving the current program as a user preset (a name typed in the header) writes its file and
//    renames the program; next & previous reach it, and loading it brings back every parameter.

#define NOMINMAX
#include <windows.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "EngineBridge.h"
#include "PluginState.h"

using namespace dtblkfix;

// Smear's random phases, shared by all instances (FxRun1_0.cpp)
extern long g_rand_i;

static int g_failed = 0;

static void check(bool ok, const char* what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok)
    g_failed++;
}

struct NoListener : public EngineListener {
  void paramChangedByEditor(int, float) override {}
  void beginEdit(int) override {}
  void endEdit(int) override {}
  void latencyChanged() override {}
  void viewChangeRequested(int, int, int) override { view_changes++; }
  void programChangedByEditor() override { program_changes++; }
  int program_changes = 0, view_changes = 0;
};

// optional: switch the editor's skin every 20 buffers while rendering
struct SkinSwitcher {
  Engine* e;
  HWND wnd;
  int n_skins, i;
  void step(int block_i)
  {
    if (!e || block_i % 20)
      return;
    e->closeEditor();
    e->openEditor(wnd, 200, i++ % n_skins);
    e->idleEditor();
  }
};

// effect slots in use (any of the 5 params not 0), as the editor shows them
static std::vector<int> usedFx(Engine& e)
{
  std::vector<int> used;
  for (int fx = 0; fx < 8; fx++)
    for (int i = 0; i < 5; i++)
      if (e.getParam(4 + 5 * fx + i) != 0.0f) {
        used.push_back(fx);
        break;
      }
  return used;
}

// editor height in view units: header (22), global controls (24), spectrograms, effect rows and
// the "+" row
static int editorHeight(Engine& e)
{
  int rows = (int)usedFx(e).size();
  return 22 + 24 + 191 + 24 * (rows + (rows < 8 ? 1 : 0));
}

// a left click at view position (x, y) in the editor's window
static void click(HWND frame, int x, int y)
{
  LPARAM pos = MAKELPARAM(x, y);
  SendMessage(frame, WM_MOUSEMOVE, 0, pos);
  SendMessage(frame, WM_LBUTTONDOWN, MK_LBUTTON, pos);
  SendMessage(frame, WM_LBUTTONUP, 0, pos);
}

static std::vector<float> render(Engine& e, SkinSwitcher* switcher = nullptr)
{
  const int n = 3 * 44100, block = 441;
  std::vector<float> l(block), r(block), out;
  g_rand_i = 1;
  e.prepare(44100.0, block);
  e.resume();
  for (int pos = 0; pos < n; pos += block) {
    if (switcher)
      switcher->step(pos / block);
    for (int i = 0; i < block; i++) {
      double t = (pos + i) / 44100.0;
      l[i] =
          (float)(0.3 * sin(2 * 3.14159265 * 220.0 * t) + 0.1 * sin(2 * 3.14159265 * 1234.5 * t));
      r[i] = (float)(0.3 * sin(2 * 3.14159265 * 330.0 * t));
    }
    const float* in[2] = {&l[0], &r[0]};
    float* o[2] = {&l[0], &r[0]};
    e.process(in, o, block, nullptr);
    out.insert(out.end(), l.begin(), l.end());
    out.insert(out.end(), r.begin(), r.end());
  }
  return out;
}

int main()
{
  // user presets go to a folder of the test's own, emptied first
  char tmp[MAX_PATH] = {0};
  GetTempPathA(MAX_PATH, tmp);
  std::string preset_dir = std::string(tmp) + "dtblkfix_test_presets";
  CreateDirectoryA(preset_dir.c_str(), NULL);
  {
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((preset_dir + "\\*.txt").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
      do
        DeleteFileA((preset_dir + "\\" + fd.cFileName).c_str());
      while (FindNextFileA(h, &fd));
      FindClose(h);
    }
  }
  SetEnvironmentVariableA("DTBLKFIX_PRESET_DIR", preset_dir.c_str());
  _putenv_s("DTBLKFIX_PRESET_DIR", preset_dir.c_str());

  if (!Engine::init()) {
    printf("FAIL: engine init (is the dtblkfx folder next to the test?)\n");
    return 1;
  }
  std::vector<std::string> skins = Engine::skinNames();
  check(skins.size() >= 2 && skins[0] == "Original", "skins: Original and the alternates");
  for (size_t i = 0; i < skins.size(); i++)
    printf("     skin %d: %s\n", (int)i, skins[i].c_str());

  NoListener listener;

  // --- 1. state round trip
  Engine a(&listener);
  a.setProgram(33);
  a.setParam(0, 0.25f);
  a.setParam(2, 0.3f);
  a.setParam(4 + 5 * 2 + 3, 0.5f);
  a.setProgramName(5, "renamed");
  ViewSettings view;
  view.scale_pct = 150;
  view.skin = skins.back();
  view.style = 1;
  std::vector<uint8_t> state = encodeState(a.getChunk(false), view);

  Engine b(&listener);
  ViewSettings view_b;
  size_t chunk_n = decodeState(state.data(), state.size(), &view_b);
  check(b.setChunk(state.data(), chunk_n, false), "state loads");

  bool params_same = a.numParams() == b.numParams();
  for (int i = 0; params_same && i < a.numParams(); i++)
    params_same = a.getParam(i) == b.getParam(i);
  check(params_same, "all parameters come back");
  check(a.getProgram() == b.getProgram(), "current program comes back");
  bool names_same = a.numPrograms() == b.numPrograms();
  for (int i = 0; names_same && i < a.numPrograms(); i++)
    names_same = a.programName(i) == b.programName(i);
  check(names_same && b.programName(5) == "renamed", "program names come back (incl. renamed)");
  check(view_b.scale_pct == 150 && view_b.skin == view.skin && view_b.style == 1,
        "editor size, skin & style come back");
  check(encodeState(b.getChunk(false), view_b) == state, "saving again gives the same bytes");
  check(a.latency() == b.latency(), "same latency");
  check(render(a) == render(b), "same audio, bit for bit");

  // switching skins (with the editor open, at 200%) doesn't change the audio
  {
    int w = 0, h = 0;
    b.editorSize(200, w, h);
    SkinSwitcher sw = {&b, (HWND)b.createEditorWindow(w, h), (int)skins.size(), 0};
    std::vector<float> with_editor = render(b, &sw);
    b.closeEditor();
    DestroyWindow(sw.wnd);
    check(with_editor == render(a), "same audio while the editor switches skins");
  }

  // --- 2. bare chunk (no size & skin trailer)
  std::vector<uint8_t> chunk = a.getChunk(false);
  ViewSettings view_c;
  check(decodeState(chunk.data(), chunk.size(), &view_c) == chunk.size() &&
            view_c.scale_pct == 100 && view_c.skin == "Original" && view_c.style == 0,
        "a bare chunk loads with 100%, Original skin and Original style");

  // --- 3. editor at each size, skin & style
  check(a.hasEditor(), "editor available");
  // the menu's sizes, a size dragged to and the largest
  std::vector<int> sizes = Engine::scales();
  sizes.push_back(137);
  sizes.push_back(Engine::maxScale());
  for (int style = 0; style < 2; style++)
  for (int scale : sizes) {
    for (int skin = 0; skin < (int)skins.size(); skin++) {
      int w = 0, h = 0;
      a.editorSize(scale, w, h);
      HWND wnd = (HWND)a.createEditorWindow(w, h);
      bool opened = a.openEditor(wnd, scale, skin, style);
      HWND frame = GetWindow(wnd, GW_CHILD);
      RECT r = {0, 0, 0, 0};
      if (frame)
        GetClientRect(frame, &r);
      for (int i = 0; i < 3; i++)
        a.idleEditor();
      a.closeEditor();
      DestroyWindow(wnd);
      char what[256];
      snprintf(what,
               sizeof(what),
               "editor at %d%%, %s skin, %s style: %dx%d",
               scale,
               skins[skin].c_str(),
               style ? "boxes" : "original",
               (int)r.right,
               (int)r.bottom);
      check(opened && r.right == w && r.bottom == h &&
                abs(w - (int)floor(410 * scale / 100.0 + 0.5)) <= 1 &&
                abs(h - (int)floor(editorHeight(a) * scale / 100.0 + 0.5)) <= 1,
            what);
    }
  }

  // the resize grip: a hole for it in the editor window's bottom right corner, & its colours
  {
    int w = 0, h = 0;
    a.editorSize(150, w, h);
    HWND wnd = (HWND)a.createEditorWindow(w, h);
    int dark = 0;
    for (int i = 0; i < (int)skins.size(); i++)
      if (skins[i] == "Dark")
        dark = i;
    a.openEditor(wnd, 150, dark, 1);
    uint32_t background = 0, label = 0;
    bool colours = a.editorColours(background, label);
    Engine::setEditorWindowCorner(wnd, w, h, 21, 21);
    HRGN rgn = CreateRectRgn(0, 0, 0, 0);
    bool hole = GetWindowRgn(wnd, rgn) != ERROR && !PtInRegion(rgn, w - 5, h - 5) &&
                !PtInRegion(rgn, w - 21, h - 21) && PtInRegion(rgn, w - 22, h - 5) &&
                PtInRegion(rgn, 5, 5);
    DeleteObject(rgn);
    a.closeEditor();
    DestroyWindow(wnd);
    check(hole, "the editor window leaves out the grip's corner");
    int lum = (int)((background >> 16) & 0xff) + (int)((background >> 8) & 0xff) +
              (int)(background & 0xff);
    check(colours && label != background && lum < 3 * 128, "grip colours from the Dark skin");
  }

  // --- 4. "+" and "x", in both styles
  for (int style = 0; style < 2; style++) {
    printf("   %s style\n", style ? "boxes" : "original");
    Engine e(&listener);
    e.setProgram(33); // Vocode
    std::vector<int> before = usedFx(e);
    float first[5];
    for (int i = 0; i < 5; i++)
      first[i] = e.getParam(4 + 5 * before[1] + i);

    int w = 0, h = 0;
    e.editorSize(100, w, h);
    HWND wnd = (HWND)e.createEditorWindow(w, h);
    e.openEditor(wnd, 100, 0, style);
    HWND frame = GetWindow(wnd, GW_CHILD);
    check(h == editorHeight(e), "editor: one row per effect in use and a + row");

    // "+" is the row after the effects
    click(frame, 205, 237 + 24 * (int)before.size() + 12);
    std::vector<int> added = usedFx(e);
    int slot = before.back() + 1;
    check(added.size() == before.size() + 1 && added.back() == slot &&
              e.getParam(4 + 5 * slot + 0) == 0.0f && e.getParam(4 + 5 * slot + 1) == 1.0f &&
              e.getParam(4 + 5 * slot + 2) == 0.6f,
          "+ adds a full range 0 dB filter after the last effect");

    // the wrapper reopens the editor after a change
    e.closeEditor();
    e.openEditor(wnd, 100, 0, style);
    frame = GetWindow(wnd, GW_CHILD);
    RECT r;
    GetClientRect(frame, &r);
    check(r.bottom == editorHeight(e), "the editor grows by a row");

    // "x" on the first row
    click(frame, 9, 237 + 12);
    std::vector<int> removed = usedFx(e);
    bool moved_up = removed.size() == before.size();
    for (int i = 0; i < 5; i++)
      moved_up = moved_up && e.getParam(4 + i) == first[i];
    check(moved_up, "x removes the effect and the ones below move up");

    // the header's next & previous preset buttons load the program and tell the wrapper
    int changes = listener.program_changes;
    click(frame, 256, 11);
    Engine ref(&listener);
    ref.setProgram(34);
    bool loaded = true;
    for (int i = 0; i < e.numParams(); i++)
      loaded = loaded && e.getParam(i) == ref.getParam(i);
    bool next_ok = e.getProgram() == 34 && loaded && listener.program_changes == changes + 1;
    click(frame, 235, 11);
    check(next_ok && e.getProgram() == 33 && listener.program_changes == changes + 2,
          "header: next preset, then previous");

    // the gear opens the settings page in place of the spectrograms (the wrapper reopens the
    // editor), and closes it again
    int views = listener.view_changes;
    click(frame, 396, 11);
    bool asked = listener.view_changes == views + 1;
    e.closeEditor();
    e.openEditor(wnd, 100, 0, style);
    frame = GetWindow(wnd, GW_CHILD);
    for (int i = 0; i < 5; i++)
      e.idleEditor();
    RECT pr;
    GetClientRect(frame, &pr);
    click(frame, 396, 11);
    e.closeEditor();
    e.openEditor(wnd, 100, 0, style);
    frame = GetWindow(wnd, GW_CHILD);
    e.idleEditor();
    check(asked && listener.view_changes == views + 2 && pr.bottom == editorHeight(e),
          "header: the gear opens the settings page and closes it");

    e.closeEditor();
    DestroyWindow(wnd);
  }

  // --- 5. saving the current program as a user preset, and loading it again
  {
    NoListener pl;
    Engine e(&pl);
    e.setProgram(33);
    e.setParam(0, 0.25f); // MixBack, so it isn't the factory preset
    std::vector<float> saved_params;
    for (int i = 0; i < e.numParams(); i++)
      saved_params.push_back(e.getParam(i));
    int w = 0, h = 0;
    e.editorSize(100, w, h);
    HWND wnd = (HWND)e.createEditorWindow(w, h);
    e.openEditor(wnd, 100, 0, 0);
    HWND frame = GetWindow(wnd, GW_CHILD);

    // save: a name box over the preset's name; Enter saves (':' isn't allowed in a name)
    click(frame, 277, 11);
    HWND edit = FindWindowExA(frame, NULL, "EDIT", NULL);
    if (edit) {
      SetWindowTextA(edit, "Test: one");
      SendMessageA(edit, WM_KEYDOWN, VK_RETURN, 0);
    }
    e.idleEditor();
    std::string file = preset_dir + "\\Test- one.txt";
    FILE* f = fopen(file.c_str(), "rb");
    char line[4096] = {0};
    if (f) {
      fgets(line, sizeof(line), f);
      fclose(f);
    }
    check(edit != NULL && f != NULL && strncmp(line, "Test- one:", 10) == 0 &&
              e.programName(33) == "Test- one" && pl.program_changes == 1 &&
              FindWindowExA(frame, NULL, "EDIT", NULL) == NULL,
          "save: the name typed becomes a user preset file, and the program's name");

    // the wrapper reopens the editor; change something, then go to the factory preset before the
    // user preset and back to the user preset
    e.closeEditor();
    e.openEditor(wnd, 100, 0, 0);
    frame = GetWindow(wnd, GW_CHILD);
    e.setParam(0, 0.75f);
    click(frame, 235, 11); // previous: the last factory preset
    bool factory = e.getProgram() != 33 && e.getParam(0) != 0.25f;
    int prog = e.getProgram();
    click(frame, 256, 11); // next: the user preset
    bool same = true;
    for (int i = 0; i < e.numParams(); i++)
      same = same && e.getParam(i) == saved_params[i];
    check(factory && same && e.getProgram() == prog && e.programName(prog) == "Test- one",
          "load: next from the last factory preset loads the user preset, every parameter");

    // the user preset is in the list after a reopen (read from its folder)
    e.closeEditor();
    e.openEditor(wnd, 100, 0, 0);
    e.idleEditor();
    e.closeEditor();
    DestroyWindow(wnd);
    DeleteFileA(file.c_str());
  }

  printf("%s\n", g_failed ? "FAILED" : "all passed");
  return g_failed ? 1 : 0;
}
