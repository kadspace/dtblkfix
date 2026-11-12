// DtBlkFix tests: the single-file VST3 in a host (JUCE's VST3 hosting, built with the plugin).
//
// dtblkfix_host_test <DtBlkFix.vst3> <work folder> [--fresh]
//
// Copies the plugin into the work folder on its own, with no "dtblkfx" folder beside it, points
// LOCALAPPDATA at the work folder and loads it, so it has to unpack its resources, then runs audio
// through it. --fresh empties the work folder first; without it, what's already unpacked must be
// used as it is.

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include <math.h>
#include <stdio.h>

static int g_failed = 0;

static void check(bool ok, const juce::String& what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what.toRawUTF8());
  if (!ok)
    g_failed++;
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    printf("usage: dtblkfix_host_test <DtBlkFix.vst3> <work folder> [--fresh]\n");
    return 2;
  }
  juce::ScopedJuceInitialiser_GUI juce_init;

  juce::File work{juce::String(argv[2])};
  bool fresh = argc > 3 && juce::String(argv[3]) == "--fresh";
  if (fresh)
    work.deleteRecursively();
  juce::File plugin_dir = work.getChildFile("VST3"), appdata = work.getChildFile("AppData");
  juce::File unpacked = appdata.getChildFile("DtBlkFix").getChildFile(DTBLKFIX_VERSION);
  juce::File stamp = unpacked.getChildFile(".pack");
  juce::Time stamp_time = stamp.getLastModificationTime();
  check(fresh != stamp.existsAsFile(), fresh ? "nothing unpacked yet" : "already unpacked");

  juce::File plugin = plugin_dir.getChildFile("DtBlkFix.vst3");
  plugin_dir.createDirectory();
  appdata.createDirectory();
  check(juce::File{juce::String(argv[1])}.copyFileTo(plugin), "plugin copied to " + plugin.getFullPathName());
  SetEnvironmentVariableA("LOCALAPPDATA", appdata.getFullPathName().toRawUTF8());

  // load it as a host does
  juce::VST3PluginFormat format;
  juce::OwnedArray<juce::PluginDescription> found;
  format.findAllTypesForFile(found, plugin.getFullPathName());
  check(found.size() == 1, "found " + juce::String(found.size()) + " plugin(s) in the file");
  if (found.isEmpty())
    return 1;
  juce::String error;
  const double rate = 44100;
  const int block = 512;
  std::unique_ptr<juce::AudioPluginInstance> fx =
      format.createInstanceFromDescription(*found[0], rate, block, error);
  check(fx != nullptr, "instance created " + error);
  if (!fx)
    return 1;
  check(fx->getName() == "DtBlkFix", "named " + fx->getName());

  // the resources it needs were unpacked, and nothing else was left behind
  for (const char* f : {"dtblkfx/libfftw3f-3.dll", "dtblkfx/stereo_presets.txt",
                        "dtblkfx/stereo_fx_bg.png", "dtblkfx/skins/Dark/stereo_fx_bg.png",
                        "dtblkfx/skins/Warm/stereo_fx_bg.png", "licenses/DtBlkFix-LICENSE.txt",
                        "licenses/THIRD_PARTY_NOTICES.md", "licenses/FFTW-COPYING.txt"})
    check(unpacked.getChildFile(f).existsAsFile(), juce::String("unpacked ") + f);
  check(appdata.getChildFile("DtBlkFix").getNumberOfChildFiles(juce::File::findFilesAndDirectories) == 1,
        "only " + unpacked.getFileName() + " in " + appdata.getChildFile("DtBlkFix").getFullPathName());
  if (!fresh)
    check(stamp.getLastModificationTime() == stamp_time, "not unpacked again");

  // audio through it: the factory presets load, and it reports one block of latency
  check(fx->getNumPrograms() > 40, juce::String(fx->getNumPrograms()) + " programs");
  fx->setPlayConfigDetails(2, 2, rate, block);
  fx->prepareToPlay(rate, block);
  check(fx->getLatencySamples() > 0, "latency " + juce::String(fx->getLatencySamples()) + " samples");
  juce::AudioBuffer<float> buf(2, block);
  juce::MidiBuffer midi;
  float peak = 0;
  bool finite = true;
  for (int n = 0, pos = 0; n < (int)(rate * 3) / block; n++) {
    for (int i = 0; i < block; i++, pos++) {
      buf.setSample(0, i, 0.5f * (float)sin(juce::MathConstants<double>::twoPi * 220 * pos / rate));
      buf.setSample(1, i, 0.5f * (float)sin(juce::MathConstants<double>::twoPi * 330 * pos / rate));
    }
    fx->processBlock(buf, midi);
    for (int c = 0; c < 2; c++)
      for (int i = 0; i < block; i++) {
        float s = buf.getSample(c, i);
        finite = finite && isfinite(s);
        peak = std::max(peak, fabsf(s));
      }
  }
  fx->releaseResources();
  check(finite, "output finite");
  check(peak > 0.01f, "output peak " + juce::String(peak, 3));
  fx.reset();

  printf("%s\n", g_failed ? "FAILED" : "all passed");
  return g_failed ? 1 : 0;
}
