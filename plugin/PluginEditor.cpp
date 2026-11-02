// DtBlkFix plugin: the original DtBlkFx editor in a JUCE editor

#include "PluginEditor.h"

#include <algorithm>

DtBlkFixEditor::DtBlkFixEditor(DtBlkFixProcessor& processor)
    : juce::AudioProcessorEditor(processor)
    , _processor(processor)
{
  setOpaque(true);
  addAndMakeVisible(_host);
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
  dtblkfix::Engine& engine = _processor.engine();
  std::vector<std::string> skins = dtblkfix::Engine::skinNames();
  int skin_idx = 0;
  for (int i = 0; i < (int)skins.size(); i++)
    if (skins[i] == _processor.view.skin)
      skin_idx = i;

  int scale_pct = _processor.view.scale_pct;
  std::vector<int> scales = dtblkfix::Engine::scales();
  if (std::find(scales.begin(), scales.end(), scale_pct) == scales.end())
    scale_pct = _processor.view.scale_pct = 100;

  engine.editorSize(scale_pct, _pix_width, _pix_height);
  void* window = engine.createEditorWindow(_pix_width, _pix_height);
  _host.setHWND(window);
  engine.openEditor(window, scale_pct, skin_idx);
  _display_scale = 0.0f;
  updateSize();
}

void DtBlkFixEditor::closeEditor()
{
  // close the original editor before its window goes
  _processor.engine().closeEditor();
  _host.setHWND(nullptr);
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
  updateSize();
}

void DtBlkFixEditor::timerCallback()
{
  updateSize();
  _processor.engine().idleEditor();
}
