#ifndef _DTBLKFIX_AEFFEDITOR_H_
#define _DTBLKFIX_AEFFEDITOR_H_
/**************************************************************************************************
DtBlkFix plugin shim: editor base class (see vst2shim.h)

This program is free software; you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

***************************************************************************************************/

#include "vst2shim.h"

class AEffEditor {
public:
  AEffEditor(AudioEffect* effect = 0)
      : effect(effect)
      , systemWindow(0)
  {
  }
  virtual ~AEffEditor() {}

  // size of the editor, valid before open()
  virtual bool getRect(ERect** rect)
  {
    *rect = 0;
    return false;
  }

  // open in the given parent window (an HWND on Windows)
  virtual bool open(void* ptr)
  {
    systemWindow = ptr;
    return false;
  }
  virtual void close() { systemWindow = 0; }
  virtual bool isOpen() { return systemWindow != 0; }

  // called regularly by the host while the editor is open
  virtual void idle() {}

  virtual bool onKeyDown(VstKeyCode& keyCode) { return false; }
  virtual bool onKeyUp(VstKeyCode& keyCode) { return false; }
  virtual bool onWheel(float distance) { return false; }
  virtual bool setKnobMode(VstInt32 val) { return false; }

protected:
  AudioEffect* effect;
  void* systemWindow;
};

#endif
