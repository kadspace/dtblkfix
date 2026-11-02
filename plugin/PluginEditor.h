// DtBlkFix plugin: the original DtBlkFx editor in a JUCE editor

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

class DtBlkFixEditor
    : public juce::AudioProcessorEditor
    , private juce::Timer {
public:
  explicit DtBlkFixEditor(DtBlkFixProcessor& p);
  ~DtBlkFixEditor() override;

  void paint(juce::Graphics& g) override;
  void resized() override;
  void parentHierarchyChanged() override;

private:
  void timerCallback() override;
  void openEditor();
  void closeEditor();
  void updateSize();

  DtBlkFixProcessor& _processor;

  // holds the window that the original editor (VSTGUI) opens in
  juce::HWNDComponent _host;

  // editor size in pixels & the display scale it was fitted for
  int _pix_width = 0, _pix_height = 0;
  float _display_scale = 0.0f;
  bool _open = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DtBlkFixEditor)
};
