#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include <map>
#include <memory>

class SoftwinView
    : public juce::Component
    , private juce::Timer {
public:
  SoftwinView();
  ~SoftwinView() override;

  void setHWND(void* window);
  void* getHWND() const { return _window; }
  void setPixelScale(float pixel_scale) { _pixel_scale = pixel_scale > 0.0f ? pixel_scale : 1.0f; }

  void paint(juce::Graphics& g) override;
  void mouseMove(const juce::MouseEvent& e) override;
  void mouseDown(const juce::MouseEvent& e) override;
  void mouseDrag(const juce::MouseEvent& e) override;
  void mouseUp(const juce::MouseEvent& e) override;
  void mouseDoubleClick(const juce::MouseEvent& e) override;
  void mouseExit(const juce::MouseEvent& e) override;
  void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override;

  void invalidatePixels(int left, int top, int right, int bottom);
  void updateNow();
  juce::Point<int> screenPixels(juce::Point<int> local_points) const;
  float pixelScale() const { return _pixel_scale; }
  void openTextEdit(void* edit, juce::Rectangle<int> pixels, const char* text, int limit, int font_px);
  void closeTextEdit(void* edit);
  juce::TextEditor* textEdit(void* edit);
  void showOverlay(void* wnd, int x, int y, int w, int h, const uint32_t* bgra);
  void hideOverlay(void* wnd);

  static SoftwinView* forWindow(void* window);
  static SoftwinView* active();

private:
  class Overlay;

  void timerCallback() override;
  void send(unsigned message, const juce::MouseEvent& e, int wheel = 0);
  unsigned buttons(const juce::MouseEvent& e) const;
  void updateImage();

  void* _window = nullptr;
  float _pixel_scale = 1.0f;
  juce::Image _image;
  juce::Rectangle<int> _dirty;
  float _wheel = 0.0f;
  bool _right_drag = false;
  std::map<void*, std::unique_ptr<juce::TextEditor>> _edits;
  std::map<void*, std::unique_ptr<Overlay>> _overlays;
  std::vector<std::unique_ptr<juce::TextEditor>> _closed_edits;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoftwinView)
};
