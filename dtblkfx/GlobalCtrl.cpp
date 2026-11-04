/**************************************************************************************************
Copyright (c) 2005-2006 Darrell Tam
email: darrell.barrell@gmail.com

GlobalCtrl is the GUI panel at the top of the DtBlkFx window that lets you change the following
MixBack : % mixback of original audio (Owner invisi horiz slider)
Delay: Number of beats delay (Owner invisi horiz slider)
Overlap: % overlap of FFT blks (Owner invisi horiz slider)
Blk Sync: whether blks are sync'd to tempo ((Owner toggle)
Blk Len: pull down menu of FFT blk sz

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

#include "BlkFxParam.h"
#include "DtBlkFx.hpp"
#include "GlobalCtrl.h"
#include "Gui.h"

#define LOG_FILE_NAME "c:\\GlobalCtrl.html"
#include "Debug.h"

//-------------------------------------------------------------------------------------------------
// get blk fx out of the main gui class
inline DtBlkFx* GlobalCtrl::blkFx()
{
  return _gui->blkFx();
}

//-------------------------------------------------------------------------------------------------
GlobalCtrl::GlobalCtrl()
    : CViewContainer(CRect(), // const CRect &rect,
                     NULL,    // CFrame *pParent,
                     NULL     // CBitmap *pBackground
      )
{
  LOGG("", "GlobalCtrl" << VAR(this));
  _ok = false;
}

//-------------------------------------------------------------------------------------------------
GlobalCtrl::~GlobalCtrl()
{
  LOGG("", "~GlobalCtrl" << VAR(this));
}

//-------------------------------------------------------------------------------------------------
template <class STR> void BlkLenText(STR& str, int samps, float sample_rate)
// DtBlkFix: block length as "92.9 ms · 4096"
{
  float ms = 1000.0f * (float)samps / std::max(sample_rate, 1.0f);
  str << sprf(ms < 10.0f ? "%.2f" : ms < 100.0f ? "%.1f" : "%.0f", ms) << " ms \xb7 " << samps;
}

//-------------------------------------------------------------------------------------------------
// DtBlkFix: the global controls are one line of label & value pairs, the same space at each end
// as between them. The values are plain text, or in boxes sized for their widest value (style
// "boxes", see Gui::boxes()).
static const struct {
  const char* label;
  const char* typ;    // a usual value, to place the controls before the first draw
  const char* widest; // the widest value, for the boxes
} g_glob_col[4] = {{"MixBack", "50%", "100%"},
                   {"Power", "match", "match"},
                   {"Overlap", "60%", "85%"},
                   {"BlkLen", "92.9 ms \xb7 4096", "1829 ms \xb7 80640"}};
static const int GLOB_FONT = 11;    // labels & values, so they share a baseline
static const int GLOB_GAP = 5;      // between label & value
static const int GLOB_BOX_PAD = 5;  // either side of the value in its box
static const int GLOB_BOX_INSET = 4; // box from the top & bottom of the strip

//-------------------------------------------------------------------------------------------------
bool /*success*/ GlobalCtrl::init(Gui* gui, CastCRect rect)
//
// expect gui to have been init'd with:
//   spectrograms
//   frame
{
  int i;
  /*CView::*/ size = rect;
  /*CView::*/ mouseableArea = rect;
  _ok = false;
  _gui = gui;

  // force menus to be rebuilt
  _sample_rate = -1.0f;
  _samps_per_beat = -1.0f;

  _text_height = 15;

  // size of each child rectangle
  // DtBlkFix: 4 columns (MixBack, Power, Overlap, BlkLen): Delay is automatic (one block, reported
  // to the host) and Sync is always on, so neither has a control any more
  //
  _mixback.New();
  _mixback->setTag(BlkFxParam::MIX_BACK);
  _mixback->setListener(this);
  AddView(_mixback, this);

  //
  _pwrmatch.New();
  _pwrmatch->setTag(BlkFxParam::MIX_BACK);
  _pwrmatch->setListener(this);
  AddView(_pwrmatch, this);

  //
  _delay.New();
  _delay->setSize(CRect(0, 0, 0, 0)); // DtBlkFix: hidden
  _delay->setMouseEnabled(false);
  _delay->setTag(BlkFxParam::DELAY);
  _delay->setListener(this);
  AddView(_delay, this);

  // overlap
  _overlap.New();
  _overlap->setTag(BlkFxParam::OVERLAP);
  _overlap->setListener(this);
  AddView(_overlap, this);

  // block sync
  _blksync.New();
  _blksync->setSize(CRect(0, 0, 0, 0)); // DtBlkFix: hidden, Sync is always on
  _blksync->setMouseEnabled(false);
  _blksync->setTag(BlkFxParam::OVERLAP);
  _blksync->setListener(this);
  AddView(_blksync, this);

  // block size
  _blksz.New();
  _blksz->setTag(BlkFxParam::FFT_LEN);
  _blksz->setListener(this);
  _blksz->setMenuSnap();
  AddView(_blksz, this);

  // do the menus that aren't done in updateMenus()
  CharArray<64> str;
  for (i = 0; i <= 100; i += 25) {
    float value = (float)i * 0.01f;
    str << spr_percent(value);
    _mixback->addMenuEntry(value, str);
  }

  // overlap menu
  _overlap->addMenuEntry(0.0f, "minimum");
  for (i = 10; i <= 80; i += 10) {
    str << i << "%";
    // note, scaled value matches overlap (0.0=>0%, 1.0=>85%)
    _overlap->addMenuEntry((float)i * (0.01f / 0.85f), str);
  }
  _overlap->addMenuEntry(1.0f, "maximum");

  // DtBlkFix: place the controls (drawRect redoes this for the actual values)
  const char* typ[4];
  for (i = 0; i < 4; i++)
    typ[i] = g_glob_col[i].typ;
  layout(typ);

  //
  updateMenus();

  _ok = true;
  return true;
}

//-------------------------------------------------------------------------------------------------
int GlobalCtrl::valueWidth(int i, const char* val)
// DtBlkFix: room for value i: its text, or its box (the same whatever the value)
{
  CContextRGBA* temp_bm = &_gui->_temp_bm;
  temp_bm->setFont(kNormalFont, GLOB_FONT, kBoldFace);
  if (_gui->boxes())
    return (int)StringWidth(temp_bm, g_glob_col[i].widest) + 2 * GLOB_BOX_PAD;
  return (int)StringWidth(temp_bm, val);
}

//-------------------------------------------------------------------------------------------------
void GlobalCtrl::layout(const char* const val[4])
// DtBlkFix: lay out the one line for these values, with the same space at each end as between
// the label & value pairs, and fit the controls to it
{
  int i, w = getWidth(), h = getHeight();
  CContextRGBA* temp_bm = &_gui->_temp_bm;
  float lbl_w[4], used = 0.0f;
  for (i = 0; i < 4; i++) {
    _val_w[i] = valueWidth(i, val[i]);
    temp_bm->setFont(kNormalFont, GLOB_FONT, kNormalFace);
    lbl_w[i] = (float)StringWidth(temp_bm, g_glob_col[i].label);
    used += lbl_w[i] + GLOB_GAP + (float)_val_w[i];
  }
  float gap = std::max((float)w - used, 0.0f) / 5.0f, pos = gap;
  int end[4];
  for (i = 0; i < 4; i++) {
    _lbl_x[i] = pos;
    pos += lbl_w[i] + GLOB_GAP;
    _val_x[i] = RndToInt(pos);
    pos += (float)_val_w[i];
    end[i] = RndToInt(pos);
    pos += gap;
  }
  // each control reaches halfway to the next
  CRect col[4];
  for (i = 0; i < 4; i++)
    col[i] = CRect(i == 0 ? 0 : (end[i - 1] + RndToInt(_lbl_x[i])) / 2,
                   0,
                   i == 3 ? w : (end[i] + RndToInt(_lbl_x[i + 1])) / 2,
                   h);
  _mixback->setSize(col[0]);
  _pwrmatch->setSize(col[1]);
  _overlap->setSize(col[2]);
  _blksz->setSize(col[3]);
}

//-------------------------------------------------------------------------------------------------
void GlobalCtrl::drawRect(CDrawContext* context, const CRect& update_rect_)
// virtual, override CViewContainer
///< called if the view should draw itself
{
  //
  CRect update_rect = Bound(update_rect_, this);

  // LOG("", "drawRect" << VAR(this) << VAR(update_rect));

  // make sure that our dirty flag is cleared
  setDirty(false);

  // temporary bitmap to draw into
  CContextRGBA* temp_bm = &_gui->_temp_bm;

  // DtBlkFix: the values, first, since the layout depends on them (see layout())
  CharArray<32> val[4];
  val[0] << RndToInt(_mixback->getValue() * 100.0f) << "%";
  val[1] << (_pwrmatch->getTentativeValue() ? "match" : "filter");
  // get guesses for these
  int guess_fft_n, guess_time_n;
  blkFx()->guessFFTLen(/*return*/ guess_fft_n, /*return*/ guess_time_n);
  int samps_fwd = BlkFxParam::getBlkShiftFwd(_overlap->getValue(), guess_time_n);
  float overlap_frac = 1.0f - ((float)samps_fwd / (float)guess_time_n);
  val[2] << RndToInt(overlap_frac * 100.0f) << "%";
  // in ms and samples (the delay is always one block, so no more "*" for a block longer than the
  // delay)
  BlkLenText(val[3], g_fft_sz[getBlkSzPlan()], blkFx()->getSampleRate());

  // a value changed width: lay out again and draw it all
  bool all = false;
  for (int i = 0; i < 4; i++)
    if (valueWidth(i, val[i]) != _val_w[i])
      all = true;
  if (all) {
    const char* v[4] = {val[0], val[1], val[2], val[3]};
    layout(v);
    update_rect = size;
  }

  // update rect relative to our corner
  CRect rel_update_rect = Offset(-TopLeft(this), update_rect);
  temp_bm->setClipRect(rel_update_rect);

  // copy background bitmap into our offscreen
  // DtBlkFix: the image has 6 labels over 2 lines; only its plain background is used, from the
  // label-free rows at the bottom, and the labels are text
  CContextRGBA* bg = _gui->_images.glob_bg;
  int bg_w = bg->getWidth(), bg_h = getHeight();
  for (int y = 0; y < bg_h; y += 5)
    bg->blend(temp_bm, CRect(0, y, bg_w, std::min(y + 5, bg_h)), CPoint(0, 21), 255, false);

  // labels & values
  const GuiPalette& pal = _gui->_palette;
  CView* view[4] = {_mixback, _pwrmatch, _overlap, _blksz};
  bool focus[4] = {_mixback->getState() != _mixback->INACTIVE,
                   _pwrmatch->getState() != _pwrmatch->INACTIVE,
                   _overlap->getState() != _overlap->INACTIVE,
                   _blksz->getState() != _blksz->INACTIVE};
  for (int i = 0; i < 4; i++) {
    if (!ShouldDrawView(update_rect, view[i]))
      continue;
    temp_bm->setFont(kNormalFont, GLOB_FONT, kNormalFace);
    temp_bm->setFontColor(pal.label);
    temp_bm->drawString(g_glob_col[i].label,
                        CRect(RndToInt(_lbl_x[i]), 0, _val_x[i], bg_h),
                        /*opaque*/ 0,
                        kLeftText);

    CRect r(_val_x[i], 0, _val_x[i] + _val_w[i], bg_h);
    if (_gui->boxes()) {
      r.top += GLOB_BOX_INSET;
      r.bottom -= GLOB_BOX_INSET;
      DrawBox(temp_bm, r, pal.box, focus[i] ? pal.hover : pal.box_line);
    }
    temp_bm->setFont(kNormalFont, GLOB_FONT, kBoldFace);
    temp_bm->setFontColor(focus[i] ? pal.hover : pal.value);
    // a box is centred on the strip, so its text shares the labels' baseline
    temp_bm->drawString(val[i],
                        _gui->boxes() ? r : CRect(r.left, 0, r.right + 10, bg_h),
                        /*opaque*/ 0,
                        _gui->boxes() ? kCenterText : kLeftText);
  }

  // blend region to update onto the screen
  context->setFillColor(CColorRGB(0, 255, 0));
  context->fillRect(update_rect);

  LOG("", "drawRect" << VAR(update_rect) << VAR(rel_update_rect));
  temp_bm->blend(context,
                 /*dst*/ update_rect,
                 /*src*/ rel_update_rect,
                 /*overall alpha*/ 255,
                 /*use src alpha*/ false);
}

//-------------------------------------------------------------------------------------------------
void GlobalCtrl::draw(CDrawContext* context)
// virtual, override CViewContainer
// draw all
{
  drawRect(context, size);
}

//-------------------------------------------------------------------------------------------------
void GlobalCtrl::updateMenus()
// update popup menus if need be
// note that only the "f_part" of blk delay is stored in the menu
{
  int i;
  CharArray<80> str;

  // check whether we need to update the beat presets
  if (blkFx()->getSampleRate() != _sample_rate || blkFx()->getSampsPerBeat() != _samps_per_beat) {
    // get latest
    _sample_rate = std::max(blkFx()->getSampleRate(), 1.0f);
    _samps_per_beat = std::max(blkFx()->getSampsPerBeat(), 1.0f);

    // beat presets
    float beat_presets[] = {.25f,
                            1 / 3.0f,
                            0.5f,
                            2 / 3.0f,
                            0.75f,
                            1.0f,
                            1.5f,
                            2.0f,
                            3.0f,
                            4.0f,
                            5.0f,
                            6.0f,
                            7.0f,
                            8.0f};

    //
    _delay_menu_units.resize(NUM_ELEMENTS(beat_presets) + 2);

    // make sure there's enough entries in the menus
    while (_delay->getNbEntries() < NUM_ELEMENTS(beat_presets) + 2)
      _delay->addMenuEntry(/*value*/ -1, /*text*/ "");

    // add beat presets to the menu (leave the first 2 free)
    for (i = 0; i < NUM_ELEMENTS(beat_presets); i++) {
      float beats = beat_presets[i];
      float samps = beats * _samps_per_beat;
      float sec = samps / _sample_rate;

      str << sprf("%.2f", beats) << " beats, "
          << "(" << (int)samps << " samps, " << sprnum(sec, /*min*/ 1e-3f) << "sec)";

      _delay->setMenuEntry(i + 2, /*value*/ BlkFxParam::Delay::beats(beats).f_part, /*text*/ str);
      _delay_menu_units[i + 2] = BlkFxParam::Delay::BEATS;
    }

    // populate blksz menu
    while (_blksz->getNbEntries() < NUM_FFT_SZ)
      _blksz->addMenuEntry(/*value*/ -1, /*text*/ "");

    for (i = 0; i < NUM_FFT_SZ; i++) {
      // DtBlkFix: ms and samples, as shown
      BlkLenText(str, g_fft_sz[i], _sample_rate);

      //
      float value = (float)i / (float)NUM_FFT_SZ;

      //
      _blksz->setMenuEntry(i, value, str);
    }
  }

  // update entry 0 in the delay menu so that you can toggle between msec & beats

  // build delay param
  BlkFxParam::Delay delay_param;
  delay_param.setUnits(_delay_units);
  delay_param.f_part = _delay->getValue(); // raw param value 0..1

  // find delay in samples
  float delay_samps = (float)(blkFx()->getDelaySamps(delay_param) - blkFx()->_initial_delay);
  float delay_secs = delay_samps / _sample_rate;
  float delay_beats = delay_samps / _samps_per_beat;
  float delay_value;

  // generate param corresponding to other units
  if (_delay_units == BlkFxParam::Delay::MSEC) {

    // delay is in msec, convert to beats
    delay_value = BlkFxParam::Delay::beats(delay_beats).f_part;

    str << sprf("%.2f beats", delay_beats) << "(" << (int)delay_samps << " samps, "
        << sprnum(delay_secs, 1e-3f) << "sec)";

    _delay_menu_units[0] = BlkFxParam::Delay::BEATS;
  }
  else {
    // delay is in beats, convert to msec
    delay_value = BlkFxParam::Delay::msec(delay_secs * 1000.0f).f_part;
    str << sprnum(delay_secs, 1e-3f) << "sec, " << (int)delay_samps << " samps, "
        << sprf("(%.2f beats)", delay_beats);

    _delay_menu_units[0] = BlkFxParam::Delay::MSEC;
  }
  _delay->setMenuEntry(/*idx*/ 0, delay_value, str);

  // update entry 1 to correspond to the current blk len
  int fft_samps = g_fft_sz[getBlkSzPlan()];
  float fft_secs = (float)fft_samps / _sample_rate;
  float fft_beats = fft_samps / _samps_per_beat;

  str << sprnum(fft_secs, 1e-3f) << "sec, " << fft_samps << " samps, "
      << sprf("(%.2f beats)", fft_beats);

  _delay->setMenuEntry(
      /*idx*/ 1, /*value*/ BlkFxParam::Delay::msec(fft_secs * 1000.0f).f_part, str);
  _delay_menu_units[1] = BlkFxParam::Delay::MSEC;
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult GlobalCtrl::onMouseDown(CPoint& mouse, const long& buttons)
// virtual, override CViewContainer
// intercept mouse down so that we can update the delay & blksz menus if need be
//
{
  // rebuild popup menus (if need be)
  //
  if (IsInside(_delay, mouse) || IsInside(_blksz, mouse))
    updateMenus();

  return CViewContainer::onMouseDown(mouse, buttons);
}

//-------------------------------------------------------------------------------------------------
bool GlobalCtrl::removed(CView* parent)
// virtual, override CViewContainer
{
  LOG("", "removed");
  CViewContainer::removed(parent);

  _ok = false;

  // could cleanup more stuff, free memory etc
  return true;
}

//-------------------------------------------------------------------------------------------------
void GlobalCtrl::valueChanged(CControl* ctrl)
// virtual, override CControlListener
//
// called by the internal gui controls when a value has changed
//
{
  // for safety
  if (!_ok)
    return;

  // tag corresponds to the param number
  int param_num = ctrl->getTag();

  // BlkFxParam
  float param_val;

  // which param was it??
  switch (param_num) {
    case BlkFxParam::MIX_BACK:
      // general param val
      param_val = BlkFxParam::getMixbackParam(_mixback->getValue(), _pwrmatch->getBoolValue());
      break;

    case BlkFxParam::DELAY: {
      // is the value from the menu? (-1 if it's from the slider)
      int menu_idx = _delay->lastMenuIdx();

      // if it is from the menu then the delay units come from the units vector
      // (while the float part comes from the slider/menu)
      if (idx_within(menu_idx, _delay_menu_units)) {
        _delay_units = _delay_menu_units[menu_idx];
        setDirty();
      }

      // build a param to send
      BlkFxParam::Delay delay_param;
      delay_param.setUnits(_delay_units);
      delay_param.f_part = _delay->getValue();
      param_val = delay_param;
    } break;

    case BlkFxParam::FFT_LEN:
      param_val = BlkFxParam::getFFTLenParam(getBlkSzPlan());
      break;

    case BlkFxParam::OVERLAP:
      param_val = BlkFxParam::getOverlapParam(_overlap->getValue(), _blksync->getBoolValue());
      break;

    default:
      // shouldn't happen
      return;
  }

  // send param to blkfx
  blkFx()->setParameterAutomated(param_num, param_val);
}

//-------------------------------------------------------------------------------------------------
void GlobalCtrl::setParameter(long index, float v)
//
// called when host application changes a parameter
//
// NOTE, we expect this to be called synchronously with "init" since we're relying on controls not
// being deleted
//
{
  LOG("", "setParameter" << VAR(index) << VAR(v) << VAR(_ok));
  if (!_ok)
    return;

  switch (index) {
    case BlkFxParam::MIX_BACK:
      _mixback->setValue(BlkFxParam::getMixBackFrac(v));
      _pwrmatch->setValue(BlkFxParam::getPwrMatch(v));
      break;

    case BlkFxParam::DELAY: {
      BlkFxParam::Delay param_val = v;

      _delay->setValue(param_val.f_part);

      if (_delay_units != param_val.getUnits()) {
        _delay_units = param_val.getUnits();
        setDirty();
      }
    } break;

    case BlkFxParam::FFT_LEN:
      _blksz->setValue((float)BlkFxParam::getPlan(v) / (float)NUM_FFT_SZ);
      break;

    case BlkFxParam::OVERLAP:
      _overlap->setValue(BlkFxParam::getOverlapPart(v));
      _blksync->setValue(BlkFxParam::getBlkSync(v));
      break;
  }
}
