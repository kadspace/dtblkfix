/**************************************************************************************************
Copyright (c) 2005-2006 Darrell Tam
email: darrell.barrell@gmail.com

Top-level DtBlkFx gui

Holds the following panels:
    1x GlobalCtrl
    2x spectrogram
    4x FxCtrl


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
#include <StdAfx.h>

#define GUI_GLOBAL_EXTERN

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <valarray>

#include "BlkFxParam.h"
#include "DTBlkFx.hpp"

#include "Gui.h"
#include "Spectrogram.h"

#define LOG_FILE_NAME "c:\\temp\\temp\\gui.html"
#include "Debug.h"

using namespace std;
using namespace VSTGUI;

enum { SGRAM_MENU_ID = 1000, SIZE_MENU_ID = 1010, SKIN_MENU_ID = 1020, STYLE_MENU_ID = 1030 };

//-------------------------------------------------------------------------------------------------
Array<const char*, 7> g_sgram_menu_strings = {"Input L+R",
                                              "Input Left",
                                              "Input Right",
                                              "Output L+R",
                                              "Output Left",
                                              "Output Right",
                                              "Paused"};

//-------------------------------------------------------------------------------------------------
// channel map matches menu string
static bool g_stereo_ch_menu_map[/*in-out*/ 2][/*menu*/ 7][/*ch*/ 2] = {
    {
        // in
        {true, true},   // L+R in
        {true, false},  // left in only
        {false, true},  // right in only
        {false, false}, // L+R out
        {false, false}, // left out only
        {false, false}, // right out only
        {false, false}  // paused
    },
    {
        // out
        {false, false}, // L+R in
        {false, false}, // left in only
        {false, false}, // right in only
        {true, true},   // L+R out
        {true, false},  // left out only
        {false, true},  // right out only
        {false, false}  // paused
    }};

#if 0
//-------------------------------------------------------------------------------------------------
void GenerateAmpOverlay(CContextRGBA* bm)
// generate exponential amplitude gfx into _amp_bm
// this code was written (instead of just using a normal bitmap) back when the whole gui was
// going to be scalable
{
	if(!bm->ok()) return;

	int width = bm->getWidth();
	int height = bm->getHeight();
	float fwidth = (float)width;
	float fheight = (float)height;

	// generate an exponential curve
	int zero_db = (int)(0.6f*fwidth);
	float xscale = 1.0f/zero_db;
	float yscale = 0.8f*fheight;
	CPoint p;
	for(int x = 0; x < width; x++) {
		float top = yscale*(1-powf(20.0f, (float)p.x*xscale)/20.0f);

		int i_top = (int)floorf(top);

		Ptr2dRGBA p = bm->get2d(x, /*y*/0);
		for(int y = 0; y < height; y++) {
			// totally transparent
			if(y < i_top)
				p.setTransparent();

			// completely covered
			else if(y != i_top)
				p.set(/*r*/255, /*g*/0, /*b*/0);

			// partially covered
			else p.set(/*r*/255, /*g*/0, /*b*/0, RndToInt(255.0f*(1.0f-top+(float)i_top)));

			p.incY();
		}
	}
}
#endif

//-------------------------------------------------------------------------------------------------
Gui::Gui(AudioEffect* effect)
    : AEffGUIEditor(effect)
{
  LOGG("", "Gui::Gui" << VAR(this));
  _ok = false;

  // DtBlkFix: original images & size until setView() is called
  _images.splash_bg = Images::g_splash_bg;
  _images.glob_bg = Images::g_glob_bg;
  _images.fx_bg = Images::g_fx_bg;
  for (int i = 0; i < 2; i++) {
    _images.slider_bg[i] = DtPopupHSlider::Images::g_bg[i];
    _images.slider_knob[i] = DtPopupHSlider::Images::g_knob[i];
  }
  _scale = 1.0f;
  _skin_idx = 0;
  _style = GUI_STYLE_ORIGINAL;
  _view_listener = NULL;
  _relayout_requested = false;

  /*AEffGUIEditor*/ frame = 0;

  /*AEffGUIEditor::rect*/
  rect.left = 0;
  rect.top = 0;
  layoutRows(); // DtBlkFix: rect.right & rect.bottom

  // build the colour gradient map for the spectrograms, each 5 element array
  // contains: position (0..1), R, G, B, A
  Array<float, 5> col_vec[] = {/*   pos     R     G     B     A*/
                               {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
                               {0.2f, 0.0f, 0.0f, 1.0f, 1.0f},
                               {0.4f, 0.0f, 1.0f, 1.0f, 1.0f},
                               {0.6f, 0.0f, 0.8f, 0.0f, 1.0f},
                               {0.8f, 1.0f, 1.0f, 0.0f, 1.0f},
                               {1.0f, 1.0f, 0.0f, 0.0f, 1.0f}};
  _col_map.resize(1000);
  GenerateGradientRGBA(_col_map, TO_RNG(col_vec));
}

//-------------------------------------------------------------------------------------------------
void Gui::setView(float scale, const GuiImages& images, const std::vector<std::string>& skin_names,
                  int skin_idx, int style, const std::vector<int>& scales_pct,
                  GuiViewListener* listener)
// DtBlkFix: UI scale, skin & style, call while closed
{
  _scale = scale;
  _images = images;
  _skin_names = skin_names;
  _skin_idx = skin_idx;
  _style = style == GUI_STYLE_BOXES ? GUI_STYLE_BOXES : GUI_STYLE_ORIGINAL;
  _scales_pct = scales_pct;
  _view_listener = listener;
}

//-------------------------------------------------------------------------------------------------
void Gui::makePalette()
// DtBlkFix: light or dark text & boxes, from the brightness of the global controls' background
{
  unsigned long c = PtrRGBA(_images.glob_bg->data())._get(); // bottom left, plain background
  _palette.dark = ((c >> 16) & 0xff) * 3 + ((c >> 8) & 0xff) * 6 + (c & 0xff) < 1280;
  if (_palette.dark) {
    _palette.label = CColorRGB(150, 146, 190);
    _palette.value = CColorRGB(238, 238, 245);
    _palette.hover = CColorRGB(255, 128, 128);
    _palette.box = CColorRGB(20, 23, 32);
    _palette.box_line = CColorRGB(74, 80, 104);
  }
  else {
    _palette.label = CColorRGB(112, 112, 124);
    _palette.value = CColorRGB(22, 22, 28);
    _palette.hover = CColorRGB(200, 40, 60);
    _palette.box = CColorRGB(252, 252, 254);
    _palette.box_line = CColorRGB(186, 188, 202);
  }
}

//-------------------------------------------------------------------------------------------------
void Gui::addViewMenus(COptionMenu* menu)
// DtBlkFix: Size, Skin & Style submenus, picked in valueChanged()
{
  CharArray<32> str;
  _size_menu.setNew(new COptionMenu(CRect(0, 0, 0, 0), this, SIZE_MENU_ID));
  _size_menu->setStyle(kCheckStyle);
  int curr_scale = -1; // a size dragged to isn't in the menu
  for (int j = 0; j < (int)_scales_pct.size(); j++) {
    str << _scales_pct[j] << "%";
    _size_menu->addEntry(str, j);
    if (_scales_pct[j] == RndToInt(_scale * 100.0f))
      curr_scale = j;
  }
  _size_menu->setCurrent(curr_scale);
  menu->addEntry(_size_menu, "Size");

  _skin_menu.setNew(new COptionMenu(CRect(0, 0, 0, 0), this, SKIN_MENU_ID));
  _skin_menu->setStyle(kCheckStyle);
  for (int j = 0; j < (int)_skin_names.size(); j++)
    _skin_menu->addEntry(_skin_names[j].c_str(), j);
  _skin_menu->setCurrent(_skin_idx);
  menu->addEntry(_skin_menu, "Skin");

  _style_menu.setNew(new COptionMenu(CRect(0, 0, 0, 0), this, STYLE_MENU_ID));
  _style_menu->setStyle(kCheckStyle);
  _style_menu->addEntry("Original", GUI_STYLE_ORIGINAL);
  _style_menu->addEntry("Boxes (easier to read)", GUI_STYLE_BOXES);
  _style_menu->setCurrent(_style);
  menu->addEntry(_style_menu, "Style");
}

//-------------------------------------------------------------------------------------------------
void Gui::selectProgram(int program)
// DtBlkFix: the listener loads it and tells the host (and reopens us: the rows may change)
{
  if (_view_listener && program >= 0 && program < blkFx()->getNumPrograms())
    _view_listener->guiProgramChange(program);
  if (_header)
    _header->setDirty();
}

//-------------------------------------------------------------------------------------------------
Gui::~Gui()
{
  LOGG("", "Gui::~Gui" << VAR(this));
}

//-------------------------------------------------------------------------------------------------
bool /*success*/ Gui::openSgram(int i, CPoint* p)
{
  LOGG("", "Gui::openSgram" << VAR(this) << VAR(i) << VAR(*p));

  // determine size of sgram based on splash background size
  CRect splash_bg_rect =
      GetTile(_images.splash_bg, /*num x*/ 1, /*num y*/ 2, /*idx x*/ 0, /*idx y*/ i);

  //
  NewView(_sgram[i], frame);

  //
  _sgram[i]->init(MoveTo(*p, splash_bg_rect), // size of window
                  _pix_hz,  // pixel to freq & width of client (Spectrogram does NOT take a copy)
                  _col_map, // colour map
                  BlkFxParam::octaveFrac() // fraction of total span per octave
  );

  //
  p->y += splash_bg_rect.getHeight();

  //
  if (!_sgram[i]->ok()) {
    LOG("background:#f00", "Gui::openSgram failed");
    return false;
  }

  // add a menu in stereo mode
  if (BlkFxParam::AUDIO_CHANNELS == 2) {
    NewView(_sgram_menu[i], _sgram[i]);

    _sgram_menu[i]->setSize(MoveToOrigin(_sgram[i]));
    _sgram_menu[i]->setTag(SGRAM_MENU_ID + i);
    _sgram_menu[i]->setListener(this);
    _sgram_menu[i]->setStyle(kCheckStyle);

    for (int j = 0; j < g_sgram_menu_strings.size(); j++)
      _sgram_menu[i]->addEntry(g_sgram_menu_strings[j], j);

    _sgram_menu[i]->setValue(i ? 3 : 0);
    _sgram[i]->setName(g_sgram_menu_strings[i ? 3 : 0]);
  }
  else {
    _sgram[i]->setName(i ? "Output" : "Input");
  }

  // draw into the sgram bitmap
  CContextRGBA* sgram_dc = _sgram[i]->getImage();

  // copy background image
  _images.splash_bg->blend(
      sgram_dc,                                                  // dest draw context
      TopLeft(_sgram[i]) - TopLeft(_sgram[i]->getDisplayRect()), // destination point
      splash_bg_rect,                                            // src rect
      255,                                                       // overall alpha
      false                                                      // use source alpha?
  );

  // write some text on the second sgram
  if (i != 1)
    return true;

  // draw some stuff into the second sgram
  sgram_dc->setFontColor(CColorRGB(255, 255, 255));
  sgram_dc->setFillColor(CColorRGB(0, 0, 0));
  sgram_dc->setFont(kNormalFont, /*sz*/ 0, kBoldFace);
  CRect txt_r(0, 10, sgram_dc->getWidth(), 10 + 15);

  const char* tip[] = {"~", "Hi there BlkFx user", "~"};
  for (int l = 0; l < 3; l++) {
    OutlineDrawString(sgram_dc, txt_r, tip[l]);
    txt_r.offset(0, 15);
  }

  CharArray<128> str;
  Rng<char> s = str;

#ifdef STEREO
  s = s << "Stereo";
#else
  s = s << "Mono";
#endif

  s = s << " (compiled " __DATE__ ")";
#ifdef _DEBUG
  s = s << " DEBUG";
#endif

  txt_r.offset(0, 5);
  OutlineDrawString(sgram_dc, txt_r, str);
  return true;
}

//-------------------------------------------------------------------------------------------------
bool Gui::open(void* system_window)
// virtual, override AEffGUIEditor::open
// open the GUI window
//
{
  int i;

  LOGG("", "Gui::open" << VAR(this) << VAR(system_window));

  _ok = false;

  // DtBlkFix: rows shown & size, colours for the skin
  layoutRows();
  makePalette();
  _relayout_requested = false;

  CRect frame_rect(0, 0, rect.right, rect.bottom);

  AEffGUIEditor::open(system_window);

  // build pixel to freq mapping
  _pix_hz.resize(frame_rect.getWidth() - 10);
  BlkFxParam::genPixelToHz(_pix_hz);

  // build pixel to bin range mapping
  for (i = 0; i < BlkFxParam::AUDIO_CHANNELS; i++)
    _pix_bin[i].init(_pix_hz, //
                     blkFx()->getSampleRate(),
                     blkFx()->FFTdata(i) // FFT data buffer
    );

  // DtBlkFix: sliders pick up their images from here when they're created
  for (i = 0; i < 2; i++) {
    DtPopupHSlider::Images::g_bg[i] = _images.slider_bg[i];
    DtPopupHSlider::Images::g_knob[i] = _images.slider_knob[i];
  }

  // temporary bitmap for children to use as an offscreen buffer
  _temp_bm.create(rect.right,
                  max(globHeight(), (int)_images.fx_bg->getHeight()),
                  /*DtBlkFix: scale*/ _scale);
  if (!_temp_bm.ok())
    return false;

  // create main window
  frame = new CFrame(frame_rect, system_window, /*gui editor*/ this);

  // make sure frame doesn't do any erasing because it flickers
  frame->setTransparency(true);

  // position of each control
  CPoint p(0, 0);

  // DtBlkFix: the header (preset & settings)
  if (headHeight() > 0) {
    NewView(_header, frame);
    _header->init(this, CRect(p.x, p.y, p.x + _images.glob_bg->getWidth(), p.y + headHeight()));
    p.y += headHeight();
  }

  // place global controls
  NewView(_glob_ctrl, frame);
  _glob_ctrl->init(/*gui*/ this,
                   /*size*/ CRect(p.x, p.y, p.x + _images.glob_bg->getWidth(), p.y + globHeight()));
  if (!_glob_ctrl->ok())
    return false;
  p.y += globHeight();

  // place spectrograms
  for (i = 0; i < 2; i++)
    if (!openSgram(i, &p))
      return false;

  _sgram[0]->linkSgram(_sgram[1]);
  _sgram[1]->linkSgram(_sgram[0]);

  // create fx ctrls
  // DtBlkFix: only for the rows shown, then the "+" row if there's room for another effect
  CRect fx_rect = GetTile(_images.fx_bg, /*num x*/ 1, /*num y*/ 5);
  for (int j = 0; j < (int)_rows.size(); j++) {
    i = _rows[j];
    NewView(_fx_ctrl[i], frame);
    _fx_ctrl[i]->init(/*gui*/ this, /*size*/ Offset(p, fx_rect), i);
    p.y += fx_rect.height();
  }
  if (editableRows() && (int)_rows.size() < BlkFxParam::NUM_FX_SETS) {
    NewView(_add_row, frame);
    _add_row->init(this, Offset(p, fx_rect));
  }

  _ok = true;

  // load params from the effect
  for (i = 0; i < BlkFxParam::TOTAL_NUM; i++)
    setParameter(i, blkFx()->getCurrParam(i));

  return true;
}

//-------------------------------------------------------------------------------------------------
void Gui::FFTDataRdy(int a // 0=input data, 1=output data
)
// preconditions:
//   DtBlkFx::_protection is locked
{
  // note that there is a potential race if the gui is open, closed & re-opened during this
  // method but I think that it being closed & reopened so quickly as to cause a problem is
  // unlikely

  if (!_ok)
    return;

  int plan = blkFx()->_plan;
  long samp_pos = blkFx()->_blk_samp_abs;
  long time_fft_n = blkFx()->_time_fft_n;
  if (BlkFxParam::AUDIO_CHANNELS == 1) {
    // mono, input data always goes into upper sgram & output into lower sgram
    if (!_sgram[a]->rdy())
      return;

    _sgram[a]->newData(_pix_bin[/*channel*/ 0].getMap(plan), /*scale*/ 1.0f);
    _sgram[a]->blkDone(samp_pos, time_fft_n);
    return;
  }

  // stereo, check which channels to display
  for (int sgram_i = 0; sgram_i < 2; sgram_i++) {
    if (!_sgram[sgram_i]->rdy())
      continue;

    // convert menu number 0..6 to 0..2
    // input data corresponds to menu values 0..2, output data to 3..5
    int menu_val = (int)(_sgram_menu[sgram_i]->getValue());

    // for safety
    if (!withinRng(menu_val, g_sgram_menu_strings))
      continue;

    // update spectrogram with audio data
    bool update = false;
    for (int ch = 0; ch < 2; ch++) {
      if (g_stereo_ch_menu_map[a][menu_val][ch]) {
        float pwr_scale = 1.0f;
        if (a == 1)
          pwr_scale = blkFx()->_chan[ch].out_pwr_scale;
        _sgram[sgram_i]->newData(_pix_bin[ch].getMap(plan), pwr_scale);
        update = true;
      }
    }

    if (update)
      _sgram[sgram_i]->blkDone(samp_pos, time_fft_n);
  }
}

#if 0
//-------------------------------------------------------------------------------------------------
bool Gui::keysRequired ()
{
	// don't expect keys for now
	//return false;

	if (frame && frame->getEditView()) return true;
	else return false;
}

//-------------------------------------------------------------------------------------------------
long Gui::onKeyDown (VstKeyCode &keyCode)
{
	if (frame && 0 && (keyCode.character >= 'a' && keyCode.character <= 'z'))
	{
		char val[64];
		char modifiers[32];
		strcpy (modifiers, "");
		if (keyCode.modifier & MODIFIER_SHIFT)
			strcpy (modifiers, "Shift+");
		if (keyCode.modifier & MODIFIER_ALTERNATE)
			strcat (modifiers, "Alt+");
		if (keyCode.modifier & MODIFIER_COMMAND)
			strcat (modifiers, "Cmd+");
		if (keyCode.modifier & MODIFIER_CONTROL)
			strcat (modifiers, "Ctrl+");

		sprintf (val, "onKeyDown : '%s%c'", modifiers, (char)(keyCode.character));
		return 1;
	}

	if (frame && (keyCode.virt == VKEY_UP || keyCode.virt == VKEY_DOWN))
	{
		CView *pView = frame->getCurrentView ();
	}
	return -1;
}

//-------------------------------------------------------------------------------------------------
long Gui::onKeyUp (VstKeyCode &keyCode)
{
	if (0 && (keyCode.character >= 'a' && keyCode.character <= 'z'))
	{
		char val[64];
		char modifiers[32];
		strcpy (modifiers, "");
		if (keyCode.modifier & MODIFIER_SHIFT)
			strcpy (modifiers, "Shift+");
		if (keyCode.modifier & MODIFIER_ALTERNATE)
			strcat (modifiers, "Alt+");
		if (keyCode.modifier & MODIFIER_COMMAND)
			strcat (modifiers, "Cmd+");
		if (keyCode.modifier & MODIFIER_CONTROL)
			strcat (modifiers, "Ctrl+");

		sprintf (val, "onKeyUp : '%s%c'", modifiers, (char)(keyCode.character));
		return 1;
	}

	return -1; 
}

#endif

//-------------------------------------------------------------------------------------------------
void Gui::resume()
// preconditions:
//   DtBlkFx::_protection is locked
{
  LOG("", "Gui::resume");
  // called when the plugin will be On
}

//-------------------------------------------------------------------------------------------------
void Gui::suspend()
// preconditions:
//   DtBlkFx::_protection is locked
{
  LOG("", "Gui::suspend");
  if (!_ok)
    return;

  // called when the plugin will be Off
  for (int i = 0; i < 2; i++)
    _sgram[i]->suspend();
}

//-------------------------------------------------------------------------------------------------
void Gui::close()
// virtual, override AEffGUIEditor::close()
// the GUI window is closing
{
  LOGG("", "Gui::close" << VAR(this) << VAR(frame));

  _ok = false;

  // assume blkfx hasn't been deleted yet, make sure that we are sync'd with it before going on
  // since it might be currently updating the sgrams (which are about to be deleted)
  {
    ScopeCriticalSection scs(blkFx()->_protect);
  }

  // release these things
  _sgram[0] = NULL;
  _sgram[1] = NULL;
  _add_row = NULL;
  _glob_ctrl = NULL;
  for (int i = 0; i < BlkFxParam::NUM_FX_SETS; i++)
    _fx_ctrl[i] = NULL;

  //
  delete /*AEffGUIEditor::*/ frame;
  frame = 0;

  //
  AEffGUIEditor::close();
}

//-------------------------------------------------------------------------------------------------
void Gui::idle()
// virtual, override AEffGUIEditor::idle()
// this is called regularly by the host app
{
  // LOG("", "Gui::idle");

  // call down (default will trigger repaint of invalid windows)
  AEffGUIEditor::idle();

  if (!_ok)
    return;

  // if (/*AEffGUIEditor::*/inIdleStuff) return;

  // inject these events so that we can catch the mouse leaving the window
  GenerateMouseMoved(frame);

  // DtBlkFix: an effect that came into use (automation, the host) gets a row
  if (editableRows() && !_relayout_requested) {
    for (int i = 0; i < BlkFxParam::NUM_FX_SETS; i++) {
      if (!_fx_ctrl[i] && fxUsed(i)) {
        requestRelayout();
        break;
      }
    }
  }

  // update the spectrograms
  CViewDrawContext dc(frame);
  for (int i = 0; i < 2; i++) {
    OffsetDrawContext(&dc, _sgram[i]);
    _sgram[i]->update(&dc);
  }
}

//-------------------------------------------------------------------------------------------------
void Gui::setParameter(long index, float v)
// virtual, override AEffGUIEditor::setParameter()
//
// called by DtBlkFx to update the control's value
{
  // test if the plug is opened
  if (!_ok)
    return;

  BlkFxParam::SplitParamNum p(index);

  if (p.glob_param >= 0)
    _glob_ctrl->setParameter(p.glob_param, v);

  else if (p.fx_param >= 0 && idx_within(p.fx_set, _fx_ctrl) && _fx_ctrl[p.fx_set])
    _fx_ctrl[p.fx_set]->setParameter(p.fx_param, v);
}

//-------------------------------------------------------------------------------------------------
void Gui::valueChanged(CControl* ctrl)
// virtual, override CControlListener
//
// handle sgram menus
{
  if (!_ok)
    return;

  // DtBlkFix: UI size, skin & style, the listener reopens us
  if (_view_listener && (ctrl->getTag() == SIZE_MENU_ID || ctrl->getTag() == SKIN_MENU_ID ||
                         ctrl->getTag() == STYLE_MENU_ID)) {
    int sel = (int)ctrl->getValue();
    int scale_pct = RndToInt(_scale * 100.0f);
    int skin_idx = _skin_idx, style = _style;
    if (ctrl->getTag() == SIZE_MENU_ID && idx_within(sel, _scales_pct))
      scale_pct = _scales_pct[sel];
    if (ctrl->getTag() == SKIN_MENU_ID && idx_within(sel, _skin_names))
      skin_idx = sel;
    if (ctrl->getTag() == STYLE_MENU_ID && (sel == GUI_STYLE_ORIGINAL || sel == GUI_STYLE_BOXES))
      style = sel;
    _view_listener->guiViewChange(scale_pct, skin_idx, style);
    return;
  }

  // handle the results of spectrogram menus
  //
  int i = ctrl->getTag() - SGRAM_MENU_ID;
  if (i < 0 || i >= 2)
    return;

  // update the spectrogram name with the menu selection
  int sel = (int)_sgram_menu[i]->getValue();

  // update sgram
  if (withinRng(sel, g_sgram_menu_strings))
    _sgram[i]->setName(g_sgram_menu_strings[sel]);
}

//*************************************************************************************************
// DtBlkFix: effect rows
//*************************************************************************************************

//-------------------------------------------------------------------------------------------------
bool Gui::fxUsed(int fx_set)
{
  int offs = BlkFxParam::paramOffs(fx_set);
  for (int i = 0; i < BlkFxParam::NUM_FX_PARAMS; i++)
    if (blkFx()->getCurrParam(offs + i) != 0.0f)
      return true;
  return false;
}

//-------------------------------------------------------------------------------------------------
void Gui::layoutRows()
// fill in _rows and the editor size
{
  _rows.clear();
  for (int i = 0; i < BlkFxParam::NUM_FX_SETS; i++)
    if (!editableRows() || fxUsed(i))
      _rows.push_back(i);

  int n_rows = (int)_rows.size();
  if (editableRows() && n_rows < BlkFxParam::NUM_FX_SETS)
    n_rows++; // "+" row

  rect.right = (VstInt16)_images.glob_bg->getWidth();
  rect.bottom = (VstInt16)(headHeight() + globHeight() + _images.splash_bg->getHeight() +
                           _images.fx_bg->getHeight() / 5 * n_rows);
}

//-------------------------------------------------------------------------------------------------
bool Gui::getRect(ERect** ppRect)
// virtual, override AEffGUIEditor
{
  if (!_ok)
    layoutRows();
  return AEffGUIEditor::getRect(ppRect);
}

//-------------------------------------------------------------------------------------------------
void Gui::requestRelayout()
{
  _relayout_requested = true;
  if (_view_listener)
    _view_listener->guiViewChange(RndToInt(_scale * 100.0f), _skin_idx, _style);
}

//-------------------------------------------------------------------------------------------------
void Gui::setFxParams(int fx_set, const float* v)
// set an effect's params as if changed in the editor (the host is told)
{
  int offs = BlkFxParam::paramOffs(fx_set);
  for (int i = 0; i < BlkFxParam::NUM_FX_PARAMS; i++) {
    if (blkFx()->getCurrParam(offs + i) == v[i])
      continue;
    blkFx()->beginEdit(offs + i);
    blkFx()->setParameterAutomated(offs + i, v[i]);
    blkFx()->endEdit(offs + i);
  }
}

//-------------------------------------------------------------------------------------------------
void Gui::addFx()
// add a do-nothing filter (full range at 0dB) after the last effect
{
  enum { N = BlkFxParam::NUM_FX_SETS, P = BlkFxParam::NUM_FX_PARAMS };

  int last = -1;
  for (int i = 0; i < N; i++)
    if (fxUsed(i))
      last = i;

  // the last slot is taken: move the effects up over the unused slots first
  if (last == N - 1) {
    float v[N][P];
    int n = 0;
    for (int i = 0; i < N; i++) {
      if (!fxUsed(i))
        continue;
      for (int j = 0; j < P; j++)
        v[n][j] = blkFx()->getCurrParam(BlkFxParam::paramOffs(i) + j);
      n++;
    }
    if (n == N)
      return; // all 8 in use
    for (int i = 0; i < N; i++) {
      if (i >= n)
        memset(v[i], 0, sizeof(v[i]));
      setFxParams(i, v[i]);
    }
    last = n - 1;
  }

  float add[P];
  add[BlkFxParam::FX_FREQ_A] = 0.0f;
  add[BlkFxParam::FX_FREQ_B] = 1.0f;
  add[BlkFxParam::FX_AMP] = BlkFxParam::getAmpParam0dB();
  add[BlkFxParam::FX_TYPE] = BlkFxParam::getEffectTypeInv(0); // Filter
  add[BlkFxParam::FX_VAL] = 0.0f;
  setFxParams(last + 1, add);

  requestRelayout();
}

//-------------------------------------------------------------------------------------------------
void Gui::removeFx(int fx_set)
// remove an effect: the ones below move up so that the chain stays as shown
{
  enum { N = BlkFxParam::NUM_FX_SETS, P = BlkFxParam::NUM_FX_PARAMS };
  if (fx_set < 0 || fx_set >= N)
    return;

  for (int i = fx_set; i < N; i++) {
    float v[P];
    for (int j = 0; j < P; j++)
      v[j] = i + 1 < N ? blkFx()->getCurrParam(BlkFxParam::paramOffs(i + 1) + j) : 0.0f;
    setFxParams(i, v);
  }

  requestRelayout();
}

//-------------------------------------------------------------------------------------------------
FxAddRow::FxAddRow()
    : CView(CRect(0, 0, 0, 0))
{
  _hover = false;
}

//-------------------------------------------------------------------------------------------------
void FxAddRow::init(Gui* gui, CastCRect rect)
{
  _gui = gui;
  /*CView::*/ size = rect;
  /*CView::*/ mouseableArea = rect;
  _hover = false;
}

//-------------------------------------------------------------------------------------------------
void FxAddRow::draw(CDrawContext* context)
{
  drawRect(context, size);
}

//-------------------------------------------------------------------------------------------------
void FxAddRow::drawRect(CDrawContext* context, const CRect& update_rect)
// an empty effect row (row-selected look when hovered) with a "+" in the middle
{
  setDirty(false);
  CContextRGBA* temp_bm = &_gui->_temp_bm;
  temp_bm->setClipRect(MoveToOrigin(this));

  CRect bg_img_rect = GetTile(_gui->_images.fx_bg, /*num x*/ 1, /*num y*/ 5, 0, 0);
  _gui->_images.fx_bg->blend(/*dest*/ temp_bm,
                             /*dest*/ CPoint(0, 0),
                             /*src rect*/ TileOffset(bg_img_rect, 0, _hover ? 1 : 0),
                             /*alpha*/ 255,
                             /*use src alpha*/ false);

  if (_gui->boxes()) {
    // a "+" button like the value boxes
    const GuiPalette& pal = _gui->_palette;
    int cx = getWidth() / 2;
    CRect r(cx - 24, 4, cx + 24, getHeight() - 4);
    DrawBox(temp_bm, r, pal.box, _hover ? pal.hover : pal.box_line);
    temp_bm->setFont(kNormalFont, /*sz*/ 15, kBoldFace);
    temp_bm->setFontColor(_hover ? pal.hover : pal.value);
    temp_bm->drawString("+", r, /*opaque*/ 0, kCenterText);
  }
  else {
    // a big "+" in the outlined text style of the rest of the editor
    temp_bm->setFont(kNormalFont, /*sz*/ 20, kBoldFace);
    temp_bm->setFillColor(CColorRGB(0, 0, 0));
    temp_bm->setFontColor(_hover ? CColorRGB(0xff, 0x80, 0x80) : CColorRGB(0xff, 0xff, 0xff));
    OutlineDrawString(temp_bm, MoveToOrigin(this), "+", kCenterText);
  }

  CViewDrawContext dc(this);
  temp_bm->blend(&dc, /*dst rect*/ this, /*src point*/ CPoint(0, 0), 255, /*src alpha*/ false);
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult FxAddRow::onMouseDown(CPoint& mouse, const long& buttons)
{
  if (buttons & kLButton)
    _gui->addFx();
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult FxAddRow::onMouseEntered(CPoint& mouse, const long& buttons)
{
  _hover = true;
  setDirty();
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult FxAddRow::onMouseExited(CPoint& mouse, const long& buttons)
{
  _hover = false;
  setDirty();
  return kMouseEventHandled;
}

//*************************************************************************************************
// DtBlkFix: the header
//*************************************************************************************************

// where things are in the header (view units, the header is 410 x 22)
static const CRect g_head_prev(4, 3, 22, 19), g_head_name(25, 3, 201, 19), g_head_next(204, 3, 222, 19);
static const int HEAD_SETTINGS_W = 78; // the Settings box, at the right

//-------------------------------------------------------------------------------------------------
HeaderButton::HeaderButton(HeaderCtrl* header, int step)
    : CView(CRect(0, 0, 0, 0))
{
  _header = header;
  _step = step;
  _hover = false;
}

void HeaderButton::setSize(CastCRect r)
{
  /*CView::*/ size = r;
  /*CView::*/ mouseableArea = r;
}

CMouseEventResult HeaderButton::onMouseDown(CPoint& mouse, const long& buttons)
{
  if (buttons & kLButton)
    _header->step(_step);
  return kMouseEventHandled;
}

CMouseEventResult HeaderButton::onMouseEntered(CPoint& mouse, const long& buttons)
{
  _hover = true;
  setDirty();
  return kMouseEventHandled;
}

CMouseEventResult HeaderButton::onMouseExited(CPoint& mouse, const long& buttons)
{
  _hover = false;
  setDirty();
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
HeaderCtrl::HeaderCtrl()
    : CViewContainer(CRect(), /*frame*/ NULL, /*background*/ NULL)
{
  _num_presets = 0;
  _reset_entry = -1;
}

//-------------------------------------------------------------------------------------------------
bool HeaderCtrl::divider(int program)
// the factory presets have "--------" programs between groups
{
  char name[256] = {0};
  _gui->blkFx()->getProgramNameIndexed(0, program, name);
  if (!name[0])
    return false;
  for (const char* c = name; *c; c++)
    if (*c != '-')
      return false;
  return true;
}

//-------------------------------------------------------------------------------------------------
void HeaderCtrl::init(Gui* gui, CastCRect rect)
{
  /*CView::*/ size = rect;
  /*CView::*/ mouseableArea = rect;
  _gui = gui;
  CRect r = MoveToOrigin(rect);

  _prev.setNew(new HeaderButton(this, -1));
  _prev->setSize(g_head_prev);
  AddView(_prev, this);
  _next.setNew(new HeaderButton(this, +1));
  _next->setSize(g_head_next);
  AddView(_next, this);

  // the presets, then (after a separator) the last program, which resets the current one
  DtBlkFx* fx = _gui->blkFx();
  int n = (int)fx->getNumPrograms();
  _num_presets = std::max(n - 1, 1);
  _preset_menu.New();
  _preset_menu->setSize(g_head_name);
  _preset_menu->setListener(this);
  // (one entry per program, so an entry's index is its program; dividers are separators)
  for (int i = 0; i < _num_presets; i++) {
    char name[256] = {0};
    fx->getProgramNameIndexed(0, i, name);
    _preset_menu->addEntry(divider(i) ? "-" : name, i);
  }
  if (n > _num_presets) {
    if (!divider(_num_presets - 1))
      _preset_menu->addEntry("-");
    _reset_entry = (int)_preset_menu->getNbEntries();
    _preset_menu->addEntry("Reset this preset");
  }
  _preset_menu->setCurrent(fx->getProgram());
  AddView(_preset_menu, this);

  _settings_menu.New();
  _settings_menu->setSize(CRect(r.right - 4 - HEAD_SETTINGS_W, 3, r.right - 4, 19));
  _settings_menu->setListener(this);
  _settings_menu->setStyle(0); // no check marks
  _gui->addViewMenus(_settings_menu);
  AddView(_settings_menu, this);
}

//-------------------------------------------------------------------------------------------------
void HeaderCtrl::step(int dir)
{
  int p = _gui->blkFx()->getProgram();
  if (p < 0 || p >= _num_presets)
    p = 0;
  for (int i = 0; i < _num_presets; i++) {
    p = (p + dir + _num_presets) % _num_presets;
    if (!divider(p))
      break;
  }
  _gui->selectProgram(p);
}

//-------------------------------------------------------------------------------------------------
void HeaderCtrl::valueChanged(CControl* ctrl)
{
  if (ctrl != _preset_menu)
    return; // the Settings submenus go to the Gui
  int sel = (int)ctrl->getValue();
  if (sel >= 0 && sel < _num_presets && !divider(sel))
    _gui->selectProgram(sel);
  else if (sel == _reset_entry) // reset the current program
    _gui->selectProgram(_num_presets);
}

//-------------------------------------------------------------------------------------------------
void HeaderCtrl::draw(CDrawContext* context)
{
  drawRect(context, size);
}

//-------------------------------------------------------------------------------------------------
void HeaderCtrl::drawRect(CDrawContext* context, const CRect& update_rect_)
{
  CRect update_rect = Bound(update_rect_, this);
  setDirty(false);
  CContextRGBA* temp_bm = &_gui->_temp_bm;
  CRect rel_update_rect = Offset(-TopLeft(this), update_rect);
  temp_bm->setClipRect(rel_update_rect);
  const GuiPalette& pal = _gui->_palette;

  // the global controls' plain background
  CContextRGBA* bg = _gui->_images.glob_bg;
  int w = getWidth(), h = getHeight();
  for (int y = 0; y < h; y += 5)
    bg->blend(temp_bm, CRect(0, y, w, std::min(y + 5, h)), CPoint(0, 21), 255, false);

  // previous & next
  for (int i = 0; i < 2; i++) {
    HeaderButton* b = i ? _next : _prev;
    CRect r = i ? g_head_next : g_head_prev;
    DrawBox(temp_bm, r, pal.box, b->_hover ? pal.hover : pal.box_line);
    DrawArrow(temp_bm,
              (float)(r.left + r.right) * 0.5f,
              (float)(r.top + r.bottom) * 0.5f,
              7.0f,
              i ? +1 : -1,
              b->_hover ? pal.hover : pal.value);
  }

  // the preset's name, with a down arrow for its list
  bool name_hot = _preset_menu->getState() != _preset_menu->INACTIVE;
  CRect r = g_head_name;
  DrawBox(temp_bm, r, pal.box, name_hot ? pal.hover : pal.box_line);
  char name[256] = {0};
  _gui->blkFx()->getProgramName(name);
  temp_bm->setFont(kNormalFont, /*sz*/ 11, kBoldFace);
  temp_bm->setFontColor(name_hot ? pal.hover : pal.value);
  temp_bm->drawString(name, CRect(r.left + 6, r.top, r.right - 14, r.bottom), 0, kLeftText);
  DrawArrow(temp_bm, (float)r.right - 8.0f, (float)(r.top + r.bottom) * 0.5f, 7.0f, 0,
            name_hot ? pal.hover : pal.label);

  // Settings
  bool set_hot = _settings_menu->getState() != _settings_menu->INACTIVE;
  r = CRect(w - 4 - HEAD_SETTINGS_W, 3, w - 4, 19);
  DrawBox(temp_bm, r, pal.box, set_hot ? pal.hover : pal.box_line);
  temp_bm->setFont(kNormalFont, /*sz*/ 11, kNormalFace);
  temp_bm->setFontColor(set_hot ? pal.hover : pal.value);
  temp_bm->drawString("Settings", CRect(r.left + 8, r.top, r.right - 14, r.bottom), 0, kLeftText);
  DrawArrow(temp_bm, (float)r.right - 8.0f, (float)(r.top + r.bottom) * 0.5f, 7.0f, 0,
            set_hot ? pal.hover : pal.label);

  temp_bm->blend(context, update_rect, rel_update_rect, 255, false);
}
