// DtBlkFix plugin: the original DtBlkFx editor in a JUCE editor

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <functional>
#include <memory>

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
  // the resize grip in the bottom right corner, in the editor's colours
  class Grip : public juce::ResizableCornerComponent {
  public:
    Grip(juce::Component* editor, juce::ComponentBoundsConstrainer* constrainer)
        : juce::ResizableCornerComponent(editor, constrainer)
    {
    }
    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& e) override;

    juce::Colour background = juce::Colours::black, lines = juce::Colours::grey;
    std::function<void()> onRelease;
  };

  void timerCallback() override;
  void openEditor();
  void closeEditor();
  void updateSize();
  void updateGrip();
  void layoutHost();
  void applyDraggedSize();
  float displayScale();
  int fitScale();

  static constexpr int gripSize = 14; // points

  DtBlkFixProcessor& _processor;

  // holds the window that the original editor (VSTGUI) opens in
  juce::HWNDComponent _host;
  std::unique_ptr<Grip> _grip;

  // editor size in pixels & the display scale it was fitted for
  int _pix_width = 0, _pix_height = 0;
  float _display_scale = 0.0f;
  bool _open = false;

  // where the editor's window is in ours, and the size it was opened at (percent)
  juce::Rectangle<int> _host_bounds;
  int _opened_pct = 0;

  // true once the user sizes the window: it keeps its size and the editor is fitted into it;
  // _dragged_ticks counts down to refitting once the sizing stops for a moment
  bool _fit = false;
  int _dragged_ticks = 0;
  // true while we set our own size (not a drag)
  bool _own_resize = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DtBlkFixEditor)
};
