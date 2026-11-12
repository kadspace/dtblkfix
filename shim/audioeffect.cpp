/**************************************************************************************************
DtBlkFix plugin shim (see vst2shim.h)

This program is free software; you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

***************************************************************************************************/

#include "aeffeditor.h"
#include "vst2shim.h"

#include <string.h>

//-------------------------------------------------------------------------------------------------
AudioEffect::AudioEffect(audioMasterCallback audioMaster, VstInt32 numPrograms, VstInt32 numParams)
    : audioMaster(audioMaster)
    , host(0)
    , editor(0)
    , sampleRate(44100.0f)
    , blockSize(1024)
    , numPrograms(numPrograms)
    , numParams(numParams)
    , curProgram(0)
{
  memset(&cEffect, 0, sizeof(cEffect));
  cEffect.object = this;
  cEffect.numPrograms = numPrograms;
  cEffect.numParams = numParams;
  cEffect.numInputs = 1;
  cEffect.numOutputs = 2;
}

//-------------------------------------------------------------------------------------------------
AudioEffect::~AudioEffect()
{
  delete editor;
}

//-------------------------------------------------------------------------------------------------
void AudioEffect::setParameterAutomated(VstInt32 index, float value)
{
  setParameter(index, value);
  if (host)
    host->parameterAutomated(index, value);
}

//-------------------------------------------------------------------------------------------------
bool AudioEffectX::ioChanged()
{
  if (!host)
    return false;
  host->ioChanged();
  return true;
}

bool AudioEffectX::sizeWindow(VstInt32 width, VstInt32 height)
{
  return host ? host->sizeWindow(width, height) : false;
}

VstInt32 AudioEffectX::canHostDo(const char* text)
{
  return host && host->canHostDo(text) ? 1 : 0;
}

bool AudioEffectX::beginEdit(VstInt32 index)
{
  if (!host)
    return false;
  host->beginEdit(index);
  return true;
}

bool AudioEffectX::endEdit(VstInt32 index)
{
  if (!host)
    return false;
  host->endEdit(index);
  return true;
}

bool AudioEffectX::updateDisplay()
{
  if (!host)
    return false;
  host->updateDisplay();
  return true;
}
