#ifndef _DTBLKFIX_VST2SHIM_H_
#define _DTBLKFIX_VST2SHIM_H_
/**************************************************************************************************
DtBlkFix plugin shim

A small stand-in for the parts of a VST 2 plugin base class that DtBlkFx and its VSTGUI code
use, so that the engine builds without the Steinberg SDK. It was written from the way the engine
calls it, not from Steinberg's headers. Nothing here is binary compatible with VST 2: the
constants and structures only have to agree with the code in this repo, and a wrapper (JUCE or
a test) acts as the host through AudioEffectHost.

This program is free software; you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

***************************************************************************************************/

#include <stddef.h>
#include <stdint.h>

// the editor code asks for the 2.1 editor extensions (begin/end edit gestures)
#ifndef VST_2_1_EXTENSIONS
#  define VST_2_1_EXTENSIONS 1
#endif

typedef int16_t VstInt16;
typedef int32_t VstInt32;
typedef intptr_t VstIntPtr;

class AudioEffect;
class AEffEditor;
struct VstKeyCode; // defined by vstgui/vstkeycode.h

// string lengths the engine truncates names to (buffers passed in must be at least this big)
enum {
  kVstMaxProgNameLen = 24,
  kVstMaxParamStrLen = 8,
  kVstMaxVendorStrLen = 64,
  kVstMaxProductStrLen = 64,
  kVstMaxEffectNameLen = 32
};

// editor rectangle in pixels
struct ERect {
  VstInt16 top, left, bottom, right;
};

// plugin category
enum VstPlugCategory { kPlugCategUnknown = 0, kPlugCategEffect, kPlugCategSynth };

// flags for VstTimeInfo::flags and for the filter passed to getTimeInfo()
enum VstTimeInfoFlags {
  kVstTransportChanged = 1 << 0,
  kVstTransportPlaying = 1 << 1,
  kVstTransportCycleActive = 1 << 2,
  kVstTransportRecording = 1 << 3,
  kVstNanosValid = 1 << 8,
  kVstPpqPosValid = 1 << 9,
  kVstTempoValid = 1 << 10,
  kVstBarsValid = 1 << 11,
  kVstCyclePosValid = 1 << 12,
  kVstTimeSigValid = 1 << 13,
  kVstSmpteValid = 1 << 14,
  kVstClockValid = 1 << 15
};

// transport and tempo information, filled in by the host
struct VstTimeInfo {
  double samplePos;     // song position in samples
  double sampleRate;    // current sample rate
  double nanoSeconds;   // system time
  double ppqPos;        // song position in quarter notes
  double tempo;         // beats per minute
  double barStartPos;   // position of the last bar start in quarter notes
  double cycleStartPos; // loop start in quarter notes
  double cycleEndPos;   // loop end in quarter notes
  VstInt32 timeSigNumerator;
  VstInt32 timeSigDenominator;
  VstInt32 smpteOffset;
  VstInt32 smpteFrameRate;
  VstInt32 samplesToNextClock;
  VstInt32 flags; // VstTimeInfoFlags
};

// what a VST 2 host would hold for a plugin; here it only carries the plugin's settings
struct AEffect {
  void* object; // the AudioEffect
  VstInt32 uniqueID;
  VstInt32 numPrograms;
  VstInt32 numParams;
  VstInt32 numInputs;
  VstInt32 numOutputs;
  VstInt32 initialDelay;
  bool canReplacing;
  bool programsAreChunks;
};

// host callback type; DtBlkFxMain.cpp only uses it to ask for the host version
typedef VstIntPtr (*audioMasterCallback)(AEffect* effect, VstInt32 opcode, VstInt32 index,
                                         VstIntPtr value, void* ptr, float opt);
enum { audioMasterVersion = 1 };

//-------------------------------------------------------------------------------------------------
// what the plugin needs from its host, implemented by the wrapper
class AudioEffectHost {
public:
  virtual ~AudioEffectHost() {}

  // transport and tempo for the buffer being processed, NULL if unknown
  virtual VstTimeInfo* getTimeInfo(VstInt32 filter) = 0;

  // the plugin's latency or channel setup changed
  virtual void ioChanged() {}

  // the editor changed a parameter (setParameter has already been called)
  virtual void parameterAutomated(VstInt32 index, float value) {}
  virtual void beginEdit(VstInt32 index) {}
  virtual void endEdit(VstInt32 index) {}

  // the editor wants a different window size
  virtual bool sizeWindow(VstInt32 width, VstInt32 height) { return false; }
  virtual bool canHostDo(const char* text) { return false; }
  virtual void updateDisplay() {}
};

//-------------------------------------------------------------------------------------------------
class AudioEffect {
public:
  AudioEffect(audioMasterCallback audioMaster, VstInt32 numPrograms, VstInt32 numParams);
  virtual ~AudioEffect(); // deletes the editor

  // set by the wrapper before processing
  void setHost(AudioEffectHost* host) { this->host = host; }
  AudioEffectHost* getHost() { return host; }

  AEffect* getAeffect() { return &cEffect; }
  AEffEditor* getEditor() { return editor; }

public: // called by the host
  virtual void open() {}
  virtual void close() {}
  virtual void suspend() {}
  virtual void resume() {}

  virtual void setSampleRate(float sampleRate) { this->sampleRate = sampleRate; }
  virtual void setBlockSize(VstInt32 blockSize) { this->blockSize = blockSize; }
  virtual float getSampleRate() { return sampleRate; }
  virtual VstInt32 getBlockSize() { return blockSize; }

  virtual void process(float** inputs, float** outputs, VstInt32 sampleFrames) {}
  virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) {}

  virtual VstInt32 getProgram() { return curProgram; }
  virtual void setProgram(VstInt32 program) { curProgram = program; }
  virtual void setProgramName(char* name) {}
  virtual void getProgramName(char* name) { *name = 0; }

  virtual void setParameter(VstInt32 index, float value) {}
  virtual float getParameter(VstInt32 index) { return 0.0f; }
  virtual void getParameterLabel(VstInt32 index, char* label) { *label = 0; }
  virtual void getParameterDisplay(VstInt32 index, char* text) { *text = 0; }
  virtual void getParameterName(VstInt32 index, char* text) { *text = 0; }

  virtual VstInt32 getChunk(void** data, bool isPreset = false) { return 0; }
  virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset = false) { return 0; }

public: // settings, read by the wrapper
  VstInt32 getUniqueID() const { return cEffect.uniqueID; }
  VstInt32 getNumPrograms() const { return numPrograms; }
  VstInt32 getNumParams() const { return numParams; }
  VstInt32 getNumInputs() const { return cEffect.numInputs; }
  VstInt32 getNumOutputs() const { return cEffect.numOutputs; }
  VstInt32 getInitialDelay() const { return cEffect.initialDelay; }

public: // called by the plugin
  virtual void setEditor(AEffEditor* editor) { this->editor = editor; }
  virtual void setUniqueID(VstInt32 id) { cEffect.uniqueID = id; }
  virtual void setNumInputs(VstInt32 n) { cEffect.numInputs = n; }
  virtual void setNumOutputs(VstInt32 n) { cEffect.numOutputs = n; }
  virtual void setInitialDelay(VstInt32 delay) { cEffect.initialDelay = delay; }
  virtual void canProcessReplacing(bool state = true) { cEffect.canReplacing = state; }
  virtual void programsAreChunks(bool state = true) { cEffect.programsAreChunks = state; }

  // set a parameter and tell the host, for changes made in the editor
  virtual void setParameterAutomated(VstInt32 index, float value);

  virtual void masterIdle() {}

protected:
  audioMasterCallback audioMaster;
  AudioEffectHost* host;
  AEffEditor* editor;
  float sampleRate;
  VstInt32 blockSize;
  VstInt32 numPrograms;
  VstInt32 numParams;
  VstInt32 curProgram;
  AEffect cEffect;
};

//-------------------------------------------------------------------------------------------------
class AudioEffectX : public AudioEffect {
public:
  AudioEffectX(audioMasterCallback audioMaster, VstInt32 numPrograms, VstInt32 numParams)
      : AudioEffect(audioMaster, numPrograms, numParams)
  {
  }

public: // called by the host
  virtual bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
  {
    return false;
  }
  virtual bool getEffectName(char* name) { return false; }
  virtual bool getVendorString(char* text) { return false; }
  virtual bool getProductString(char* text) { return false; }
  virtual VstInt32 getVendorVersion() { return 0; }
  virtual VstPlugCategory getPlugCategory() { return kPlugCategUnknown; }

public: // called by the plugin
  virtual VstTimeInfo* getTimeInfo(VstInt32 filter) { return host ? host->getTimeInfo(filter) : 0; }
  virtual bool ioChanged();
  virtual bool sizeWindow(VstInt32 width, VstInt32 height);
  virtual VstInt32 canHostDo(const char* text);
  virtual bool beginEdit(VstInt32 index);
  virtual bool endEdit(VstInt32 index);
  virtual bool updateDisplay();
};

#endif
