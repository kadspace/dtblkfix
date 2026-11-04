// DtBlkFix plugin: the engine as seen from the JUCE wrapper.
//
// The engine's headers bring in windows.h, VSTGUI and "using namespace std", which don't mix
// with JUCE, so the wrapper only sees this interface (EngineBridge.cpp is the engine side).

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace dtblkfix {

// tempo & song position for a buffer
struct SongPos {
  bool tempo_valid = false, ppq_valid = false, playing = false;
  double tempo = 120.0, ppq = 0.0;
  int sig_num = 4, sig_den = 4;
};

// what the engine tells the wrapper
class EngineListener {
public:
  virtual ~EngineListener() {}
  virtual void paramChangedByEditor(int index, float value) = 0;
  virtual void beginEdit(int index) = 0;
  virtual void endEdit(int index) = 0;
  virtual void latencyChanged() = 0;
  // Size, Skin or Style picked in the editor, or its rows changed (called from inside the
  // editor's event handling)
  virtual void viewChangeRequested(int scale_pct, int skin_idx, int style) = 0;
};

class Engine {
public:
  // load FFTW, presets & images from the "dtblkfx" folder next to the plugin (once), false if the
  // engine can't run
  static bool init();

  // skins: "Original" (the dtblkfx folder itself) and each folder in dtblkfx\skins
  static std::vector<std::string> skinNames();

  // UI sizes offered, in percent
  static std::vector<int> scales();

  explicit Engine(EngineListener* listener);
  ~Engine();

  // parameters, 0..1
  int numParams() const;
  float getParam(int index);
  void setParam(int index, float value);
  std::string paramName(int index);
  std::string paramDisplay(int index);

  // programs (factory presets, editable)
  int numPrograms() const;
  int getProgram();
  void setProgram(int index);
  std::string programName(int index);
  void setProgramName(int index, const std::string& name);

  // the original chunk format (all programs, or one program for isPreset)
  std::vector<uint8_t> getChunk(bool is_preset);
  bool setChunk(const void* data, size_t size, bool is_preset);

  // audio
  void prepare(double sample_rate, int max_block);
  void suspend();
  void resume();
  void process(const float* const* in, float* const* out, int n, const SongPos* pos);
  int latency() const;

  // editor: size in pixels at a scale, open in a window made by createEditorWindow()
  bool hasEditor() const;
  void editorSize(int scale_pct, int& width, int& height) const;
  void* createEditorWindow(int width, int height);
  bool openEditor(void* window, int scale_pct, int skin_idx, int style = 0);
  void closeEditor();
  void idleEditor();

private:
  struct Impl;
  Impl* impl;
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
};

} // namespace dtblkfix
