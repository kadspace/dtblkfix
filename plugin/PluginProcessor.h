// DtBlkFix plugin: JUCE processor around the DtBlkFx engine

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <functional>
#include <memory>

#include "EngineBridge.h"
#include "PluginState.h"

class DtBlkFixProcessor
    : public juce::AudioProcessor
    , private dtblkfix::EngineListener
    , private juce::AsyncUpdater {
public:
  DtBlkFixProcessor();
  ~DtBlkFixProcessor() override;

  // audio
  void prepareToPlay(double sample_rate, int max_block) override;
  void releaseResources() override;
  void reset() override;
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
  using juce::AudioProcessor::processBlock;

  // editor
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  // info
  const juce::String getName() const override { return JucePlugin_Name; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  // programs
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& name) override;

  // state: the original chunk plus the editor's size & skin (see PluginState.h)
  void getStateInformation(juce::MemoryBlock& dest) override;
  void setStateInformation(const void* data, int size) override;

public:
  dtblkfix::Engine& engine() { return *_engine; }
  bool engineOk() const { return _engine_ok; }

  // editor size, skin & style, saved in the state
  dtblkfix::ViewSettings view;

  // called (on the message thread) to reopen the editor: Size or Skin picked, an effect added or
  // removed, a new program or state
  std::function<void()> onViewChange;

private: // EngineListener
  void paramChangedByEditor(int index, float value) override;
  void beginEdit(int index) override;
  void endEdit(int index) override;
  void latencyChanged() override;
  void viewChangeRequested(int scale_pct, int skin_idx, int style) override;
  void programChangedByEditor() override;

  // AsyncUpdater: tell the editor about a view change
  void handleAsyncUpdate() override;

private:
  bool _engine_ok;
  std::unique_ptr<dtblkfix::Engine> _engine;
  std::atomic<bool> _latency_changed{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DtBlkFixProcessor)
};
