// DtBlkFix tests: plugin state and editor through the wrapper's engine side (EngineBridge).
//
// dtblkfix_bridge_test   (needs the "dtblkfx" folder next to it, as the plugin does)
//
// 1. State round trip: settings, programs, renamed program, editor size & skin come back, the
//    state is the same bytes again and renders the same audio.
// 2. A bare DtBlkFx chunk (as the VST2 version saves) loads with the default size & skin.
// 3. The editor opens at every size & skin, at the right size.

#define NOMINMAX
#include <windows.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

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
  void viewChangeRequested(int, int) override {}
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
  check(view_b.scale_pct == 150 && view_b.skin == view.skin, "editor size & skin come back");
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
            view_c.scale_pct == 100 && view_c.skin == "Original",
        "a bare chunk loads with 100% and Original");

  // --- 3. editor at each size & skin
  check(a.hasEditor(), "editor available");
  for (int scale : Engine::scales()) {
    for (int skin = 0; skin < (int)skins.size(); skin++) {
      int w = 0, h = 0;
      a.editorSize(scale, w, h);
      HWND wnd = (HWND)a.createEditorWindow(w, h);
      bool opened = a.openEditor(wnd, scale, skin);
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
               "editor at %d%%, %s skin: %dx%d",
               scale,
               skins[skin].c_str(),
               (int)r.right,
               (int)r.bottom);
      check(opened && r.right == w && r.bottom == h &&
                abs(w - (int)floor(410 * scale / 100.0 + 0.5)) <= 1,
            what);
    }
  }

  printf("%s\n", g_failed ? "FAILED" : "all passed");
  return g_failed ? 1 : 0;
}
