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

// DtBlkFix: how the values are drawn: as in the original (outlined text over the images), or in
// boxes (easier to read)
enum { GUI_STYLE_ORIGINAL = 0, GUI_STYLE_BOXES = 1 };

// DtBlkFix: text & box colours, light or dark to suit the skin
struct GuiPalette {
  bool dark;
  CColor label, value, hover, box, box_line;
};

// DtBlkFix: the UI size, skin or style was picked from a spectrogram menu, or the effect rows
// changed; the editor should be reopened (afterwards, not from inside this call)
class GuiViewListener {
public:
  virtual void guiViewChange(int scale_pct, int skin_idx, int style) = 0;
  // DtBlkFix: a program (preset) was picked in the editor's header; load it and tell the host
  virtual void guiProgramChange(int program) = 0;
  // DtBlkFix: a user preset was loaded into the current program, or the current program saved as
  // one (its name changed): tell the host, and reopen the editor
  virtual void guiPresetLoaded() = 0;
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
// DtBlkFix: the header above the global controls: the preset (number, name to pick from a list,
// previous & next) and a gear for the settings page. Its controls are owner-drawn; the header
// draws them.
class HeaderCtrl;
class HeaderButton : public CView {
public:
  HeaderButton(HeaderCtrl* header, int step); // step -1 previous, +1 next, 0 the gear, 2 save
  void setSize(CastCRect r);
  virtual void draw(CDrawContext* /*context*/) {}
  virtual CMouseEventResult onMouseDown(CPoint& mouse, const long& buttons);
  virtual CMouseEventResult onMouseEntered(CPoint& mouse, const long& buttons);
  virtual CMouseEventResult onMouseExited(CPoint& mouse, const long& buttons);

  _Ptr<HeaderCtrl> _header;
  int _step;
  bool _hover;
};

class HeaderCtrl
    : public CViewContainer
    , public CControlListener {
public:
  HeaderCtrl();
  void init(Gui* gui, CastCRect rect);

  virtual void draw(CDrawContext* context);
  virtual void drawRect(CDrawContext* context, const CRect& update_rect);
  virtual void valueChanged(CControl* ctrl);

  // the previous or next preset: the factory presets (skipping the "--------" dividers between
  // groups of them), then the user presets
  void step(int dir);
  bool divider(int program);
  std::vector<int> bank(); // in order: a program, or -(user preset + 1)
  int bankPos();           // where the current preset is in bank(), or -1

  _Ptr<Gui> _gui;
  VstGuiRef<HeaderButton> _prev, _next, _gear, _save;
  VstGuiRef<COptionMenuOwnerDraw> _preset_menu;
  int _num_presets; // programs that are presets (the last program resets the current one)
  // what each entry of the preset list does: a program, -(user preset + 2), RESET_ENTRY, or -1
  std::vector<int> _entry;
  enum { RESET_ENTRY = -1000000 };
};

//-----------------------------------------------------------------------------
// DtBlkFix: the settings page, in place of the spectrograms while the gear is on: Size, Skin and
// Style, each a list that applies when picked (the menus go to the Gui, the page draws them)
class SettingsPage : public CViewContainer {
public:
  SettingsPage();
  void init(Gui* gui, CastCRect rect);
  virtual void draw(CDrawContext* context);
  virtual void drawRect(CDrawContext* context, const CRect& update_rect);

  _Ptr<Gui> _gui;
  VstGuiRef<COptionMenuOwnerDraw> _menu[3]; // size, skin, style
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
               int skin_idx, int style, const std::vector<int>& scales_pct,
               GuiViewListener* listener);

  // override VSTGUIEditorInterface: frame & offscreen bitmaps are scaled by this
  virtual float getScale() const { return _scale; }

  // window size in pixels
  int getScaledWidth() const { return (int)(rect.right * _scale + 0.5f); }
  int getScaledHeight() const { return (int)(rect.bottom * _scale + 0.5f); }

  GuiImages _images;
  float _scale;
  std::vector<std::string> _skin_names;
  int _skin_idx;
  int _style;
  std::vector<int> _scales_pct;
  GuiViewListener* _view_listener;
  GuiPalette _palette;

  bool boxes() const { return _style == GUI_STYLE_BOXES; }

  // pick the palette for the skin (from its background)
  void makePalette();

  // the settings page (in place of the spectrograms), opened & closed by the header's gear; it
  // stays open while the editor is reopened (a setting picked there reopens it)
  bool _settings_open;
  VstGuiRef<SettingsPage> _settings_page;
  void toggleSettings();

  // the header (preset & settings), above the global controls
  VstGuiRef<HeaderCtrl> _header;
  int headHeight() { return editableRows() ? 22 : 0; }

  // load a program picked in the header (the host is told)
  void selectProgram(int program);

  // user presets: a text file each, "<name>:<param> <param> ..." as the factory presets, in
  // Documents\DtBlkFix\Presets (or $DTBLKFIX_PRESET_DIR)
  struct UserPreset {
    std::string name, file;
  };
  std::vector<UserPreset> _user_presets; // sorted by name, read when the editor opens
  int _user_preset;                      // the one last loaded or saved, or -1
  static std::string userPresetDir(bool create);
  void readUserPresets();
  bool loadUserPreset(int k);
  bool saveUserPreset(const char* name);

  // typing a name to save the current program as: an edit box over the preset's name (Enter
  // saves, Escape or clicking elsewhere cancels; acted on in idle())
  void startNameEdit();
  void endNameEdit();
  void* _name_edit; // HWND
  int _name_edit_result; // 0 typing, 1 save, 2 cancel

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

  // height of the global controls: one line, labels beside the values
  int globHeight() { return 24; }

  // effect slots shown, in order
  std::vector<int> _rows;
  VstGuiRef<FxAddRow> _add_row;
  bool _relayout_requested;

protected:
  void requestRelayout();
  void setFxParams(int fx_set, const float* v /*NUM_FX_PARAMS*/);
};

#endif
