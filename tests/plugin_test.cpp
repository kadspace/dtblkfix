// DtBlkFix tests: the JUCE plugin processor (built with the plugin, needs JUCE).
//
// dtblkfix_plugin_test   (needs the "dtblkfx" folder next to it, as the plugin does)
//
// Parameters map 1:1 to the engine's, and a state round trip through the processor brings back
// every parameter, the program and program names, the editor size, skin and style, and the
// latency.

#include "PluginProcessor.h"

#include <stdio.h>

static int g_failed = 0;

static void check(bool ok, const juce::String& what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what.toRawUTF8());
  if (!ok)
    g_failed++;
}

static std::unique_ptr<DtBlkFixProcessor> create()
{
  return std::unique_ptr<DtBlkFixProcessor>(dynamic_cast<DtBlkFixProcessor*>(createPluginFilter()));
}

int main()
{
  juce::ScopedJuceInitialiser_GUI juce_init;

  auto a = create();
  check(a != nullptr && a->engineOk(), "processor created, engine loaded");
  if (!a || !a->engineOk())
    return 1;

  // parameters: same count, order, names and values as the engine
  auto& params = a->getParameters();
  bool same = params.size() == a->engine().numParams() && params.size() == 44;
  for (int i = 0; same && i < params.size(); i++)
    same = params[i]->getName(64) == juce::String(a->engine().paramName(i)) &&
           params[i]->getValue() == a->engine().getParam(i);
  check(same,
        "44 parameters, 1:1 with the engine (" + params[0]->getName(16) + ", " +
            params[4]->getName(16) + ", ...)");
  params[0]->setValueNotifyingHost(0.25f);
  check(a->engine().getParam(0) == 0.25f, "setting a parameter sets the engine's");

  // programs
  check(a->getNumPrograms() == a->engine().numPrograms() && a->getNumPrograms() > 40,
        "factory programs: " + juce::String(a->getNumPrograms()));
  a->setCurrentProgram(33);
  check(a->getCurrentProgram() == 33 && a->getProgramName(33) == "Vocode", "program 33 is Vocode");

  // state round trip, with size & skin
  a->prepareToPlay(44100.0, 512);
  params[2]->setValueNotifyingHost(0.4f); // block length
  params[4 + 5 * 1 + 3]->setValueNotifyingHost(0.6f);
  a->changeProgramName(5, "renamed");
  a->view.scale_pct = 150;
  a->view.skin = "Dark";
  a->view.style = 1;
  juce::MemoryBlock state;
  a->getStateInformation(state);

  auto b = create();
  b->setStateInformation(state.getData(), (int)state.getSize());
  bool values = true;
  for (int i = 0; i < params.size(); i++)
    values = values && b->getParameters()[i]->getValue() == params[i]->getValue();
  check(values, "all parameter values come back");
  check(b->getCurrentProgram() == 33, "current program comes back");
  bool names = true;
  for (int i = 0; i < a->getNumPrograms(); i++)
    names = names && b->getProgramName(i) == a->getProgramName(i);
  check(names && b->getProgramName(5) == "renamed", "program names come back");
  check(b->view.scale_pct == 150 && b->view.skin == "Dark" && b->view.style == 1,
        "editor size 150%, Dark skin & boxes style come back");
  check(b->getLatencySamples() == a->getLatencySamples() && a->getLatencySamples() > 0,
        "latency comes back: " + juce::String(b->getLatencySamples()));
  juce::MemoryBlock state_b;
  b->getStateInformation(state_b);
  check(state_b == state, "saving again gives the same bytes");

  // the editor opens at the saved size
  {
    std::unique_ptr<juce::AudioProcessorEditor> editor(b->createEditor());
    int w = 0, h = 0;
    b->engine().editorSize(150, w, h);
    // one row per effect in use plus the "+" row
    int rows = 0;
    for (int fx = 0; fx < 8; fx++)
      for (int i = 0; i < 5; i++)
        if (b->engine().getParam(4 + 5 * fx + i) != 0.0f) {
          rows++;
          break;
        }
    int view_h = 24 + 191 + 24 * (rows + (rows < 8 ? 1 : 0));
    check(editor != nullptr && w == 615 && h == (int)(view_h * 1.5f + 0.5f),
          "editor created for 615x" + juce::String(h) + " pixels (" + juce::String(rows) +
              " effects)");
  }

  printf("%s\n", g_failed ? "FAILED" : "all passed");
  return g_failed ? 1 : 0;
}
