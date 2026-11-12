// DtBlkFix plugin: JUCE processor around the DtBlkFx engine

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {

// one plugin parameter per engine parameter, same order, names and values
class EngineParam : public juce::AudioProcessorParameterWithID {
public:
  EngineParam(dtblkfix::Engine& engine, int index)
      : juce::AudioProcessorParameterWithID(juce::ParameterID("p" + juce::String(index), 1),
                                            juce::String(engine.paramName(index)))
      , _engine(engine)
      , _index(index)
      , _default(engine.getParam(index))
  {
  }

  float getValue() const override { return _engine.getParam(_index); }
  void setValue(float value) override { _engine.setParam(_index, value); }
  float getDefaultValue() const override { return _default; }

  // the engine formats its current value (e.g. "1.51kHz", "match 25%")
  juce::String getText(float value, int max_len) const override
  {
    juce::String text = value == getValue() ? juce::String(_engine.paramDisplay(_index))
                                            : juce::String(juce::roundToInt(value * 100.0f)) + "%";
    return text.substring(0, max_len);
  }
  float getValueForText(const juce::String& text) const override
  {
    return juce::jlimit(0.0f, 1.0f, text.getFloatValue() / 100.0f);
  }

private:
  dtblkfix::Engine& _engine;
  int _index;
  float _default;
};

} // namespace

//-------------------------------------------------------------------------------------------------
DtBlkFixProcessor::DtBlkFixProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
  _engine_ok = dtblkfix::Engine::init();
  _engine.reset(new dtblkfix::Engine(this));
  for (int i = 0; i < _engine->numParams(); i++)
    addParameter(new EngineParam(*_engine, i));
  setLatencySamples(_engine->latency());
}

DtBlkFixProcessor::~DtBlkFixProcessor()
{
  cancelPendingUpdate();
}

//-------------------------------------------------------------------------------------------------
void DtBlkFixProcessor::prepareToPlay(double sample_rate, int max_block)
{
  _engine->prepare(sample_rate, max_block);
  _engine->resume();
  setLatencySamples(_engine->latency());
}

void DtBlkFixProcessor::releaseResources()
{
  _engine->suspend();
}

void DtBlkFixProcessor::reset()
{
  _engine->suspend();
  _engine->resume();
}

bool DtBlkFixProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
         layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void DtBlkFixProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
  juce::ScopedNoDenormals no_denormals;
  int n = buffer.getNumSamples();
  if (n <= 0 || buffer.getNumChannels() < 2)
    return;

  // tempo & song position for Sync
  dtblkfix::SongPos pos;
  bool have_pos = false;
  if (auto* play_head = getPlayHead()) {
    if (auto info = play_head->getPosition()) {
      have_pos = true;
      if (auto bpm = info->getBpm()) {
        pos.tempo_valid = *bpm > 0.0;
        pos.tempo = *bpm;
      }
      if (auto ppq = info->getPpqPosition()) {
        pos.ppq_valid = true;
        pos.ppq = *ppq;
      }
      if (auto sig = info->getTimeSignature()) {
        pos.sig_num = sig->numerator;
        pos.sig_den = sig->denominator;
      }
      pos.playing = info->getIsPlaying();
    }
  }

  const float* in[2] = {buffer.getReadPointer(0), buffer.getReadPointer(1)};
  float* out[2] = {buffer.getWritePointer(0), buffer.getWritePointer(1)};
  _engine->process(in, out, n, have_pos ? &pos : nullptr);

  for (int ch = 2; ch < buffer.getNumChannels(); ch++)
    buffer.clear(ch, 0, n);

  if (_latency_changed.exchange(false))
    setLatencySamples(_engine->latency());
}

//-------------------------------------------------------------------------------------------------
juce::AudioProcessorEditor* DtBlkFixProcessor::createEditor()
{
  if (_engine->hasEditor())
    return new DtBlkFixEditor(*this);
  return new juce::GenericAudioProcessorEditor(*this);
}

bool DtBlkFixProcessor::hasEditor() const
{
  return true;
}

//-------------------------------------------------------------------------------------------------
int DtBlkFixProcessor::getNumPrograms()
{
  return _engine->numPrograms();
}

int DtBlkFixProcessor::getCurrentProgram()
{
  return _engine->getProgram();
}

void DtBlkFixProcessor::setCurrentProgram(int index)
{
  if (index < 0 || index >= getNumPrograms())
    return;
  _engine->setProgram(index);
  triggerAsyncUpdate(); // the editor's effect rows follow the program
  updateHostDisplay(juce::AudioProcessorListener::ChangeDetails()
                        .withProgramChanged(true)
                        .withParameterInfoChanged(true));
}

const juce::String DtBlkFixProcessor::getProgramName(int index)
{
  return _engine->programName(index);
}

void DtBlkFixProcessor::changeProgramName(int index, const juce::String& name)
{
  _engine->setProgramName(index, name.toStdString());
}

//-------------------------------------------------------------------------------------------------
void DtBlkFixProcessor::getStateInformation(juce::MemoryBlock& dest)
{
  std::vector<uint8_t> state = dtblkfix::encodeState(_engine->getChunk(false), view);
  dest.replaceAll(state.data(), state.size());
}

void DtBlkFixProcessor::setStateInformation(const void* data, int size)
{
  if (!data || size <= 0)
    return;
  dtblkfix::ViewSettings new_view;
  size_t chunk_size = dtblkfix::decodeState((const uint8_t*)data, (size_t)size, &new_view);
  _engine->setChunk(data, chunk_size, false);
  setLatencySamples(_engine->latency());

  // reopen the editor: size, skin, style and effect rows may all have changed
  view = new_view;
  triggerAsyncUpdate();
  updateHostDisplay(juce::AudioProcessorListener::ChangeDetails()
                        .withProgramChanged(true)
                        .withParameterInfoChanged(true));
}

//-------------------------------------------------------------------------------------------------
void DtBlkFixProcessor::paramChangedByEditor(int index, float value)
{
  if (auto* param = getParameters()[index])
    param->sendValueChangedMessageToListeners(value);
}

void DtBlkFixProcessor::beginEdit(int index)
{
  if (auto* param = getParameters()[index])
    param->beginChangeGesture();
}

void DtBlkFixProcessor::endEdit(int index)
{
  if (auto* param = getParameters()[index])
    param->endChangeGesture();
}

void DtBlkFixProcessor::latencyChanged()
{
  // may be on the audio thread, report it after the buffer (or now if not processing)
  _latency_changed = true;
  if (juce::MessageManager::existsAndIsCurrentThread() && _latency_changed.exchange(false))
    setLatencySamples(_engine->latency());
}

void DtBlkFixProcessor::viewChangeRequested(int scale_pct, int skin_idx, int style)
{
  // called from inside the editor's own event handling (Size, Skin, Style, + or x): reopen it
  // afterwards
  std::vector<std::string> skins = dtblkfix::Engine::skinNames();
  view.scale_pct = scale_pct;
  if (skin_idx >= 0 && skin_idx < (int)skins.size())
    view.skin = skins[skin_idx];
  view.style = style;
  triggerAsyncUpdate();
}

void DtBlkFixProcessor::programChangedByEditor()
{
  // a preset picked in the editor's header (already loaded): reopen the editor afterwards, its
  // effect rows follow the program
  triggerAsyncUpdate();
  updateHostDisplay(juce::AudioProcessorListener::ChangeDetails()
                        .withProgramChanged(true)
                        .withParameterInfoChanged(true));
}

void DtBlkFixProcessor::handleAsyncUpdate()
{
  if (onViewChange)
    onViewChange();
}

//-------------------------------------------------------------------------------------------------
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new DtBlkFixProcessor();
}
