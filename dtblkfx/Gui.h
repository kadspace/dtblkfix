/**************************************************************************************************
Copyright (c) 2005-2006 Darrell Tam
email: darrell.barrell@gmail.com

Top level DtBlkFx GUI class
please see Gui.cpp for more information

History
        Date       Version    Programmer         Comments
        16/2/03    1.0        Darrell Tam		Created


This program is free software; you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

***************************************************************************************************/

#ifndef __DT_BLKFX_Gui__
#define __DT_BLKFX_Gui__

#include "VstGuiSupport.h"
#include "misc_stuff.h"

#include <vstgui/aeffguieditor.h>

#include "BlkFxParam.h"
#include "GlobalCtrl.h"
#include "Spectrogram.h"

#include "FxCtrl.h"

// gui elements used by FxCtrl
// amplify overlay graphics

#ifndef GUI_GLOBAL_EXTERN
#  define GUI_GLOBAL_EXTERN extern
#endif

namespace Images {
GUI_GLOBAL_EXTERN VstGuiRef<CContextRGBA>
    // splash background in sgrams
    g_splash_bg,
    // globals controls background
    g_glob_bg,
    // effect background, tiled:
    g_fx_bg;
};

// DtBlkFix: the images that a Gui draws with (a skin, at the UI scale)
struct GuiImages {
  VstGuiRef<CContextRGBA> splash_bg, glob_bg, fx_bg;
  Array<VstGuiRef<CContextRGBA>, 2> slider_bg, slider_knob; // index COARSE or FINE
};

// DtBlkFix: the UI size or skin was picked from a spectrogram menu, or the effect rows changed;
// the editor should be reopened (afterwards, not from inside this call)
class GuiViewListener {
public:
  virtual void guiViewChange(int scale_pct, int skin_idx) = 0;
};

class Gui;

//-----------------------------------------------------------------------------
// DtBlkFix: the "+" row below the effects, adds an effect
class FxAddRow : public CView {
public:
  FxAddRow();
  void init(Gui* gui, CastCRect rect);

  virtual void draw(CDrawContext* context);
  virtual void drawRect(CDrawContext* context, const CRect& update_rect);
  virtual CMouseEventResult onMouseDown(CPoint& mouse, const long& buttons);
  virtual CMouseEventResult onMouseEntered(CPoint& mouse, const long& buttons);
  virtual CMouseEventResult onMouseExited(CPoint& mouse, const long& buttons);

  _Ptr<Gui> _gui;
  bool _hover;
};

//-----------------------------------------------------------------------------
class Gui
    : public AEffGUIEditor
    , public CControlListener {
public:
  Gui(AudioEffect* effect);
  virtual ~Gui();

  DtBlkFx* blkFx() { return (DtBlkFx*)effect; }

public:                 // methods called by DtBlkFx class
  void FFTDataRdy(int a // 0=input data, 1=output data
  );

  void suspend();
  void resume();
  // bool keysRequired ();

public:                         // override AEffGUIEditor methods
  virtual bool open(void* ptr); ///< Open editor, pointer to parent windows is platform-dependent
                                ///< (HWND on Windows, WindowRef on Mac).
  virtual void close();         ///< Close editor (detach from parent window)
  virtual void setParameter(long index, float value);
  virtual void idle();

  // VST 2.1
  // virtual long onKeyDown (VstKeyCode &keyCode);
  // virtual long onKeyUp (VstKeyCode &keyCode);

public
    : // override virtual methods from CControlListener (so we can get events from the pop-up menus)
  virtual void valueChanged(CControl* ctrl);

public: // internal stuff
  bool openSgram(int i, CPoint* p);

  // gradient colour map to draw spectrum with
  std::valarray<unsigned long> _col_map;

  // pixel to frequency (Hz) mapping
  std::valarray<float> _pix_hz;

  // pixel to bin mappings
  Array<PixelFreqBin, BlkFxParam::AUDIO_CHANNELS> _pix_bin;

  // global controls
  VstGuiRef<GlobalCtrl> _glob_ctrl;

  // spectrograms for input & output (for mono) or various in stereo
  Array<VstGuiRef<Spectrogram>, 2> _sgram;

  // sgram menus
  Array<VstGuiRef<COptionMenuOwnerDraw>, 2> _sgram_menu;

  // fx controls
  Array<VstGuiRef<FxCtrl>, BlkFxParam::NUM_FX_SETS> _fx_ctrl;

  // temporary bitmap for children to use
  CContextRGBA _temp_bm;

  // true if gui is open ok
  bool _ok;

public: // DtBlkFix: UI scale and skin
  // set before open(): scale (1.0 = original size), images loaded for that scale, the skin names
  // & scales to offer in the spectrogram menus and who to tell when one is picked
  void setView(float scale, const GuiImages& images, const std::vector<std::string>& skin_names,
               int skin_idx, const std::vector<int>& scales_pct, GuiViewListener* listener);

  // override VSTGUIEditorInterface: frame & offscreen bitmaps are scaled by this
  virtual float getScale() const { return _scale; }

  // window size in pixels
  int getScaledWidth() const { return (int)(rect.right * _scale + 0.5f); }
  int getScaledHeight() const { return (int)(rect.bottom * _scale + 0.5f); }

  GuiImages _images;
  float _scale;
  std::vector<std::string> _skin_names;
  int _skin_idx;
  std::vector<int> _scales_pct;
  GuiViewListener* _view_listener;

  // Size & Skin submenus of the spectrogram menus
  Array<VstGuiRef<COptionMenu>, 2> _size_menu, _skin_menu;

public: // DtBlkFix: only the effects in use get a row, with "+" to add one and "x" to remove one
  // (only with a view listener; otherwise all 8 rows, as in the original)
  bool editableRows() const { return _view_listener != NULL; }

  // an effect slot is in use if any of its params isn't 0 (an untouched slot is all 0)
  bool fxUsed(int fx_set);

  // work out which rows to show and the editor size
  void layoutRows();

  // add an effect after the last one, remove one (the effects below move up)
  void addFx();
  void removeFx(int fx_set);

  // override AEffGUIEditor: size depends on the rows
  virtual bool getRect(ERect** rect);

  // effect slots shown, in order
  std::vector<int> _rows;
  VstGuiRef<FxAddRow> _add_row;
  bool _relayout_requested;

protected:
  void requestRelayout();
  void setFxParams(int fx_set, const float* v /*NUM_FX_PARAMS*/);
};

#endif
