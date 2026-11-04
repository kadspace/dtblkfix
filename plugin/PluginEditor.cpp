// DtBlkFix plugin: the original DtBlkFx editor in a JUCE editor

#include "PluginEditor.h"

#include <cstdlib>

DtBlkFixEditor::DtBlkFixEditor(DtBlkFixProcessor& p)
    : juce::AudioProcessorEditor(p)
    , _processor(p)
{
  setOpaque(true);
  addAndMakeVisible(_host);

  // drag the corner (or the host's window edge) to resize, keeping the editor's shape; the size
  // limits & shape are set in updateSize()
  setResizable(true, false);
  _grip = std::make_unique<Grip>(this, getConstrainer());
  _grip->setAlwaysOnTop(true);
  _grip->onRelease = [this] { applyDraggedSize(); };
  addAndMakeVisible(*_grip);

  // size before there's a window, the editor opens once there is
  _processor.engine().editorSize(_processor.view.scale_pct, _pix_width, _pix_height);
  setSize(_pix_width, _pix_height);
  openEditor();

  // Size, Skin or Style picked in the editor, its rows changed, or a new state loaded: reopen
  _processor.onViewChange = [this] {
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

  _processor.view.scale_pct = juce::jlimit(
      dtblkfix::Engine::minScale(), dtblkfix::Engine::maxScale(), _processor.view.scale_pct);
  int scale_pct = _processor.view.scale_pct;

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
  }

  _display_scale = 0.0f;
  updateSize();
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
  // the original editor is sized in pixels, JUCE sizes in points
  float scale = (float)juce::Component::getApproximateScaleFactorForComponent(this);
  if (scale <= 0.0f)
    scale = 1.0f;
  if (scale == _display_scale)
    return;
  _display_scale = scale;

  // it can be dragged between the smallest & largest sizes, keeping its shape (which depends on
  // the effect rows shown)
  dtblkfix::Engine& engine = _processor.engine();
  int min_w = 0, min_h = 0, max_w = 0, max_h = 0;
  engine.editorSize(dtblkfix::Engine::minScale(), min_w, min_h);
  engine.editorSize(dtblkfix::Engine::maxScale(), max_w, max_h);
  if (auto* constrainer = getConstrainer()) {
    constrainer->setFixedAspectRatio((double)_pix_width / (double)juce::jmax(1, _pix_height));
    constrainer->setSizeLimits(juce::roundToInt(min_w / scale),
                               juce::roundToInt(min_h / scale),
                               juce::roundToInt(max_w / scale),
                               juce::roundToInt(max_h / scale));
  }

  _own_resize = true;
  setSize(juce::roundToInt(_pix_width / scale), juce::roundToInt(_pix_height / scale));
  _own_resize = false;
}

void DtBlkFixEditor::applyDraggedSize()
{
  _dragged_ticks = 0;
  int pct = _dragged_pct;
  _dragged_pct = 0;
  if (pct != 0 && pct != _processor.view.scale_pct) {
    // reopen at the new size, drawn sharp at that size, and saved with the state
    _processor.view.scale_pct = pct;
    closeEditor();
    openEditor();
  }
  else {
    // back to the exact size of the editor
    _display_scale = 0.0f;
    updateSize();
  }
}

void DtBlkFixEditor::paint(juce::Graphics& g)
{
  g.fillAll(juce::Colours::black);
}

void DtBlkFixEditor::resized()
{
  _host.setBounds(getLocalBounds());

  // the grip, and a hole for it in the editor's window (which would cover it)
  const int grip = 14;
  if (_grip)
    _grip->setBounds(getWidth() - grip, getHeight() - grip, grip, grip);
  if (!_open || _display_scale <= 0.0f)
    return;
  float s = _display_scale;
  dtblkfix::Engine::setEditorWindowCorner(_host.getHWND(),
                                          juce::roundToInt(getWidth() * s),
                                          juce::roundToInt(getHeight() * s),
                                          juce::roundToInt(grip * s));

  // dragged (the grip or the host's window edge): the size it's at, in percent of the original
  // (a host rounding our own size by a pixel isn't a drag)
  if (_own_resize || std::abs(getWidth() - juce::roundToInt(_pix_width / s)) <= 2)
    return;
  int w100 = 0, h100 = 0;
  _processor.engine().editorSize(100, w100, h100);
  int pct = juce::jlimit(dtblkfix::Engine::minScale(),
                         dtblkfix::Engine::maxScale(),
                         juce::roundToInt(getWidth() * s * 100.0f / (float)juce::jmax(1, w100)));
  _dragged_pct = pct;
  _dragged_ticks = 6; // apply when it's been still for 0.2 s
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
