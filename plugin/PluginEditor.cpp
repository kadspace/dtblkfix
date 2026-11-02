// DtBlkFix plugin: the original DtBlkFx editor in a JUCE editor

#include "PluginEditor.h"

#include <algorithm>

DtBlkFixEditor::DtBlkFixEditor(DtBlkFixProcessor& p)
    : juce::AudioProcessorEditor(p)
    , _processor(p)
{
  setOpaque(true);
  addAndMakeVisible(_host);

  // size before there's a window, the editor opens once there is
  _processor.engine().editorSize(_processor.view.scale_pct, _pix_width, _pix_height);
  setSize(_pix_width, _pix_height);
  openEditor();

  // Size or Skin picked in the editor (or a new state loaded): reopen at the new size
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

  std::vector<int> scales = dtblkfix::Engine::scales();
  if (std::find(scales.begin(), scales.end(), _processor.view.scale_pct) == scales.end())
    _processor.view.scale_pct = 100;
  int scale_pct = _processor.view.scale_pct;

  engine.editorSize(scale_pct, _pix_width, _pix_height);
  {
    // create it with the same DPI awareness as JUCE's window, so that it can be its child
    juce::ScopedThreadDPIAwarenessSetter dpi_awareness(peer->getNativeHandle());
    void* window = engine.createEditorWindow(_pix_width, _pix_height);
    _host.setHWND(window);
    engine.openEditor(window, scale_pct, skin_idx);
  }
  _open = true;
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
  setSize(juce::roundToInt(_pix_width / scale), juce::roundToInt(_pix_height / scale));
}

void DtBlkFixEditor::paint(juce::Graphics& g)
{
  g.fillAll(juce::Colours::black);
}

void DtBlkFixEditor::resized()
{
  _host.setBounds(getLocalBounds());
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
}
