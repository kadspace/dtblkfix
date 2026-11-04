// DtBlkFix plugin: the original DtBlkFx editor in a JUCE editor

#include "PluginEditor.h"

#include <cmath>
#include <cstdlib>

DtBlkFixEditor::DtBlkFixEditor(DtBlkFixProcessor& p)
    : juce::AudioProcessorEditor(p)
    , _processor(p)
{
  setOpaque(true);
  addAndMakeVisible(_host);

  // drag the corner (or the host's window edge) to any size: the editor is fitted into it (the
  // largest size that fits, centred); the size limits are set in updateSize()
  setResizable(true, false);
  _grip = std::make_unique<Grip>(this, getConstrainer());
  _grip->setAlwaysOnTop(true);
  _grip->onRelease = [this] { applyDraggedSize(); };
  addAndMakeVisible(*_grip);

  // size before there's a window, the editor opens once there is
  _processor.engine().editorSize(_processor.view.scale_pct, _pix_width, _pix_height);
  setSize(_pix_width, _pix_height);
  openEditor();

  // Size, Skin or Style picked in the editor, its rows changed, or a new state loaded: reopen.
  // A new size (picked, or loaded) sets the window's size again; otherwise a window the user
  // sized keeps its size and the editor is fitted into it.
  _processor.onViewChange = [this] {
    if (_processor.view.scale_pct != _opened_pct)
      _fit = false;
    closeEditor();
    openEditor();
  };

  // hosts used to idle VST2 editors, the spectrograms update from this
  startTimerHz(30);
}

DtBlkFixEditor::~DtBlkFixEditor()
{
  stopTimer();
  _processor.onViewChange = nullptr;
  closeEditor();
}

void DtBlkFixEditor::openEditor()
{
  // the original editor needs a native window to live in
  auto* peer = getPeer();
  if (_open || peer == nullptr)
    return;

  dtblkfix::Engine& engine = _processor.engine();
  std::vector<std::string> skins = dtblkfix::Engine::skinNames();
  int skin_idx = 0;
  for (int i = 0; i < (int)skins.size(); i++)
    if (skins[i] == _processor.view.skin)
      skin_idx = i;

  if (_fit)
    _processor.view.scale_pct = fitScale();
  _processor.view.scale_pct = juce::jlimit(
      dtblkfix::Engine::minScale(), dtblkfix::Engine::maxScale(), _processor.view.scale_pct);
  int scale_pct = _processor.view.scale_pct;
  _opened_pct = scale_pct;

  engine.editorSize(scale_pct, _pix_width, _pix_height);
  {
    // create it with the same DPI awareness as JUCE's window, so that it can be its child
    juce::ScopedThreadDPIAwarenessSetter dpi_awareness(peer->getNativeHandle());
    void* window = engine.createEditorWindow(_pix_width, _pix_height);
    _host.setHWND(window);
    engine.openEditor(window, scale_pct, skin_idx, _processor.view.style);
  }
  _open = true;

  // the grip in the editor's colours
  uint32_t background = 0, label = 0;
  if (engine.editorColours(background, label)) {
    _grip->background = juce::Colour(0xff000000u | background);
    _grip->lines = juce::Colour(0xff000000u | label);
    _grip->repaint();
    repaint();
  }

  _display_scale = 0.0f;
  updateSize();
}

float DtBlkFixEditor::displayScale()
{
  // the original editor is sized in pixels, JUCE sizes in points
  float scale = (float)juce::Component::getApproximateScaleFactorForComponent(this);
  return scale > 0.0f ? scale : 1.0f;
}

int DtBlkFixEditor::fitScale()
{
  // the largest size (percent) at which the editor, with its current rows, fits the window
  int w100 = 0, h100 = 0;
  _processor.engine().editorSize(100, w100, h100);
  float s = displayScale();
  float fx = (float)getWidth() * s * 100.0f / (float)juce::jmax(1, w100);
  float fy = (float)getHeight() * s * 100.0f / (float)juce::jmax(1, h100);
  return juce::jlimit(dtblkfix::Engine::minScale(),
                      dtblkfix::Engine::maxScale(),
                      (int)std::floor(juce::jmin(fx, fy)));
}

void DtBlkFixEditor::layoutHost()
{
  // the editor's window, centred in ours
  float s = _display_scale > 0.0f ? _display_scale : displayScale();
  int w = juce::roundToInt(_pix_width / s), h = juce::roundToInt(_pix_height / s);
  _host_bounds = juce::Rectangle<int>(juce::jmax(0, (getWidth() - w) / 2),
                                      juce::jmax(0, (getHeight() - h) / 2),
                                      w,
                                      h);
  _host.setBounds(_host_bounds);
}

void DtBlkFixEditor::closeEditor()
{
  if (!_open)
    return;
  // close the original editor before its window goes
  _processor.engine().closeEditor();
  _host.setHWND(nullptr);
  _open = false;
}

void DtBlkFixEditor::updateSize()
{
  float scale = displayScale();
  if (scale == _display_scale)
    return;
  _display_scale = scale;

  // free form: any width & height from the smallest editor (with its current rows) to well past
  // the largest
  dtblkfix::Engine& engine = _processor.engine();
  int min_w = 0, min_h = 0, max_w = 0, max_h = 0;
  engine.editorSize(dtblkfix::Engine::minScale(), min_w, min_h);
  engine.editorSize(dtblkfix::Engine::maxScale(), max_w, max_h);
  if (auto* constrainer = getConstrainer()) {
    constrainer->setFixedAspectRatio(0.0);
    constrainer->setSizeLimits(juce::roundToInt(min_w / scale),
                               juce::roundToInt(min_h / scale),
                               juce::roundToInt(max_w * 2 / scale),
                               juce::roundToInt(max_h * 2 / scale));
  }

  // the window is the editor's size, unless the user sized it
  if (!_fit) {
    _own_resize = true;
    setSize(juce::roundToInt(_pix_width / scale), juce::roundToInt(_pix_height / scale));
    _own_resize = false;
  }
  layoutHost();
  updateGrip();
}

void DtBlkFixEditor::updateGrip()
{
  // the grip, and a hole for it in the editor's window (which would cover it); the window is new
  // each time the editor opens, so this is done then too, not only when the size changes
  if (_grip)
    _grip->setBounds(getWidth() - gripSize, getHeight() - gripSize, gripSize, gripSize);
  if (!_open || _display_scale <= 0.0f)
    return;
  // the grip's part of the editor's window, from that window's bottom right corner (the editor is
  // centred, so it's there or nowhere)
  float s = _display_scale;
  auto overlap = _host_bounds.getIntersection(
      juce::Rectangle<int>(getWidth() - gripSize, getHeight() - gripSize, gripSize, gripSize));
  dtblkfix::Engine::setEditorWindowCorner(_host.getHWND(),
                                          _pix_width,
                                          _pix_height,
                                          juce::roundToInt(overlap.getWidth() * s),
                                          juce::roundToInt(overlap.getHeight() * s));
}

void DtBlkFixEditor::applyDraggedSize()
{
  // reopen at the size that fits, drawn sharp at that size, and saved with the state
  _dragged_ticks = 0;
  if (_fit && fitScale() != _opened_pct) {
    closeEditor();
    openEditor();
  }
}

void DtBlkFixEditor::paint(juce::Graphics& g)
{
  // around the editor when the window isn't its shape: the skin's background
  g.fillAll(_grip ? _grip->background : juce::Colours::black);
}

void DtBlkFixEditor::resized()
{
  layoutHost();
  updateGrip();
  if (_own_resize || !_open || _display_scale <= 0.0f)
    return;

  // sized by the user (the grip or the host's window edge); a host rounding our own size by a
  // pixel isn't that
  float s = _display_scale;
  if (!_fit && std::abs(getWidth() - juce::roundToInt(_pix_width / s)) <= 2 &&
      std::abs(getHeight() - juce::roundToInt(_pix_height / s)) <= 2)
    return;
  _fit = true;
  if (fitScale() != _opened_pct)
    _dragged_ticks = 6; // refit when it's been still for 0.2 s (or the grip is let go)
}

void DtBlkFixEditor::parentHierarchyChanged()
{
  openEditor();
  updateSize();
}

void DtBlkFixEditor::timerCallback()
{
  openEditor();
  updateSize();
  if (_open)
    _processor.engine().idleEditor();

  // a dragged size, once the dragging stops (or when the grip is let go)
  if (_dragged_ticks > 0 && !_grip->isMouseButtonDown() && --_dragged_ticks == 0)
    applyDraggedSize();
}

//-------------------------------------------------------------------------------------------------
void DtBlkFixEditor::Grip::paint(juce::Graphics& g)
{
  // three diagonal lines, brighter while the mouse is on it
  g.fillAll(background);
  g.setColour(lines.withAlpha(isMouseOverOrDragging() ? 1.0f : 0.7f));
  float w = (float)getWidth(), h = (float)getHeight();
  for (int i = 1; i <= 3; i++) {
    float d = w * (float)i / 4.0f;
    g.drawLine(w - d, h - 1.0f, w - 1.0f, h - d, 1.2f);
  }
}

void DtBlkFixEditor::Grip::mouseUp(const juce::MouseEvent& e)
{
  juce::ResizableCornerComponent::mouseUp(e);
  if (onRelease)
    onRelease();
}
