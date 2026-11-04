#include <StdAfx.h>

#include <math.h>

#include "VstGuiSupport.h"

// define these
Array<VstGuiRef<CContextRGBA>, 2> DtPopupHSlider::Images::g_bg, DtPopupHSlider::Images::g_knob;

// windows DLL instance (needed for creating windows) - also used by VSTGUI
extern void* hInstance;

//-------------------------------------------------------------------------------------------------
bool ShouldDrawView(CastCRect update_rect, CView* v)
// return true if the view "v" needs to be redrawn and possibly clear its dirty flag
// if it is fully redrawn (be sure to call this only once per draw per view)
//
{
  // if the view is not dirty, only redraw if it overlaps update_rect
  if (!v->isDirty())
    return update_rect.rectOverlap(GetRect(v));

  // otherwise view is dirty

  // only clear dirty flag if it can be completely redrawn
  if (IsContained(update_rect, v))
    v->setDirty(false);

  // redraw
  return true;
}

//-------------------------------------------------------------------------------------------------
void DiscardUnprocessedMouseClick()
// discard a pending mouse click
{
  MSG msg;
  // HWND hwnd = (HWND)getFrame ()->getSystemWindow ();
  PeekMessage(&msg, NULL /*hwnd*/, WM_LBUTTONDOWN, WM_MBUTTONDBLCLK, PM_REMOVE);
}

//-------------------------------------------------------------------------------------------------
bool /*true=ok*/ AlphaBlend(CDrawContext* dst_context,
                            CastCRect dst_rect,              // destination rectangle
                            const CDrawContext* src_context, // must have alpha channel & data must
                                                             // have been correctly pre-scaled
                            CastCRect src_rect,              // source rectangle
                            unsigned char overall_alpha,     // 255 for max
                            bool src_alpha                   // whether to use alpha channel in src
)
// alpha blend function using VST gui's function pointer to alphablend
{
  dst_rect = Offset(dst_context->offset, dst_rect);
  src_rect = Offset(src_context->offset, src_rect);

  // DtBlkFix: in pixels, either context may be scaled
  return ScaledBlt(
      dst_context, dst_rect, src_context, src_rect, /*rop*/ 0, overall_alpha, src_alpha);
}

//-------------------------------------------------------------------------------------------------
bool /*true=ok*/ ScaledBlt(CDrawContext* dst_context, CRect dst_rect,
                           const CDrawContext* src_context, CRect src_rect, unsigned long rop,
                           unsigned char overall_alpha, bool src_alpha)
// DtBlkFix: copy or alpha blend in pixels between contexts that may have GDI world transforms
// (UI scaling): find the pixel rectangles, drop the transforms for the blit and put them back
{
  // get AlphaBlend out of vstgui.cpp
  extern BOOL(WINAPI * pfnAlphaBlend)(HDC hdcDest,                // handle to destination DC
                                      int nXOriginDest,           // x-coord of upper-left corner
                                      int nYOriginDest,           // y-coord of upper-left corner
                                      int nWidthDest,             // destination fwdith
                                      int nHeightDest,            // destination fheight
                                      HDC hdcSrc,                 // handle to source DC
                                      int nXOriginSrc,            // x-coord of upper-left corner
                                      int nYOriginSrc,            // y-coord of upper-left corner
                                      int nWidthSrc,              // source fwdith
                                      int nHeightSrc,             // source fheight
                                      BLENDFUNCTION blendFunction // alpha-blending function
  );
  HDC dst = (HDC)dst_context->getSystemContext();
  HDC src = (HDC)src_context->getSystemContext();
  if (!dst || !src)
    return false;

  // pixel rectangles
  POINT d[2] = {{dst_rect.left, dst_rect.top}, {dst_rect.right, dst_rect.bottom}};
  POINT s[2] = {{src_rect.left, src_rect.top}, {src_rect.right, src_rect.bottom}};
  LPtoDP(dst, d, 2);
  LPtoDP(src, s, 2);

  // blit without transforms
  XFORM dst_xf, src_xf;
  bool dst_adv = GetGraphicsMode(dst) == GM_ADVANCED, src_adv = GetGraphicsMode(src) == GM_ADVANCED;
  if (dst_adv) {
    GetWorldTransform(dst, &dst_xf);
    ModifyWorldTransform(dst, NULL, MWT_IDENTITY);
  }
  if (src_adv && src != dst) {
    GetWorldTransform(src, &src_xf);
    ModifyWorldTransform(src, NULL, MWT_IDENTITY);
  }

  int dw = d[1].x - d[0].x, dh = d[1].y - d[0].y;
  int sw = s[1].x - s[0].x, sh = s[1].y - s[0].y;
  bool stretch = dw != sw || dh != sh;
  BOOL ok = FALSE;

  if (rop) {
    if (stretch) {
      int prv_mode = SetStretchBltMode(dst, HALFTONE);
      POINT prv_org;
      SetBrushOrgEx(dst, 0, 0, &prv_org);
      ok = StretchBlt(dst, d[0].x, d[0].y, dw, dh, src, s[0].x, s[0].y, sw, sh, rop);
      SetStretchBltMode(dst, prv_mode);
      SetBrushOrgEx(dst, prv_org.x, prv_org.y, NULL);
    }
    else
      ok = BitBlt(dst, d[0].x, d[0].y, dw, dh, src, s[0].x, s[0].y, rop);
  }
  else if (pfnAlphaBlend) {
    BLENDFUNCTION bf; // structure for alpha blending
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.AlphaFormat = src_alpha ? AC_SRC_ALPHA : 0; // use source alpha
    bf.SourceConstantAlpha = overall_alpha;        // use constant alpha
    ok = (*pfnAlphaBlend)(dst, d[0].x, d[0].y, dw, dh, src, s[0].x, s[0].y, sw, sh, bf);
  }

  if (dst_adv)
    SetWorldTransform(dst, &dst_xf);
  if (src_adv && src != dst)
    SetWorldTransform(src, &src_xf);

  return ok ? true : false;
}

//-------------------------------------------------------------------------------------------------
void OutlineDrawString(CDrawContext* context,
                       CastCRect r,     // rectangle to place txt
                       const char* str, // int str_len,
                       CHoriTxtAlign horiz_align
                       // int vert_align = 0			//0=top, 1=middle, 2=bottom
)
//
// use font colour for outside and fill colour for inside, this is not an efficient version
// can someone write a better one??
//
{
  // not very efficient but simple
  context->drawString(str, Offset(CPoint(-1, 0), r), /*opaque*/ 0, horiz_align);
  context->drawString(str, Offset(CPoint(1, 0), r), /*opaque*/ 0, horiz_align);
  context->drawString(str, Offset(CPoint(0, -1), r), /*opaque*/ 0, horiz_align);
  context->drawString(str, Offset(CPoint(0, 1), r), /*opaque*/ 0, horiz_align);
  CColor font_col = context->getFontColor();
  context->setFontColor(context->getFillColor());
  context->drawString(str, r, /*opaque*/ 0, horiz_align);
  context->setFontColor(font_col);
}

//-------------------------------------------------------------------------------------------------
CCoord StringWidth(CDrawContext* context, const char* str)
{
#ifdef _WIN32
  HDC dc = (HDC)context->getSystemContext();
  HGDIOBJ font = context->getFont() ? (HGDIOBJ)context->getFont()->getPlatformFont() : NULL;
  HGDIOBJ old_font = font ? SelectObject(dc, font) : NULL;
  SIZE size = {0, 0};
  GetTextExtentPoint32A(dc, str, (int)strlen(str), &size);
  if (old_font)
    SelectObject(dc, old_font);
  return (CCoord)size.cx;
#else
  return context->getStringWidth(str);
#endif
}

//-------------------------------------------------------------------------------------------------
void DrawBox(CDrawContext* context, CRect r, const CColor& fill, const CColor& line, int radius)
{
#ifdef _WIN32
  HDC dc = (HDC)context->getSystemContext();
  r.offset(context->offset.h, context->offset.v);
  HBRUSH brush = CreateSolidBrush(RGB(fill.red, fill.green, fill.blue));
  HPEN pen = CreatePen(PS_SOLID, /*width: one pixel whatever the scale*/ 0,
                       RGB(line.red, line.green, line.blue));
  HGDIOBJ old_brush = SelectObject(dc, brush), old_pen = SelectObject(dc, pen);
  RoundRect(dc, r.left, r.top, r.right, r.bottom, radius * 2, radius * 2);
  SelectObject(dc, old_brush);
  SelectObject(dc, old_pen);
  DeleteObject(brush);
  DeleteObject(pen);
#else
  context->setFillColor(fill);
  context->setFrameColor(line);
  context->drawRect(r, kDrawFilledAndStroked);
#endif
}

//-------------------------------------------------------------------------------------------------
void DrawArrow(CDrawContext* context, float x, float y, float size, int dir, const CColor& col)
{
#ifdef _WIN32
  // in hundredths of a view unit under a scaled world transform, so it's placed to a fraction
  // of a pixel at any scale
  HDC dc = (HDC)context->getSystemContext();
  XFORM old_xf;
  bool advanced = GetGraphicsMode(dc) == GM_ADVANCED && GetWorldTransform(dc, &old_xf);
  float k = 0.01f;
  if (advanced) {
    XFORM f = {k, 0, 0, k, 0, 0};
    ModifyWorldTransform(dc, &f, MWT_LEFTMULTIPLY);
  }
  else
    k = 1.0f;
  float ox = (x + (float)context->offset.h) / k, oy = (y + (float)context->offset.v) / k;
  float h = size * 0.5f / k;
  POINT pts[3];
  if (dir == 0) { // down
    pts[0].x = (LONG)(ox - h); pts[0].y = (LONG)(oy - h * 0.6f);
    pts[1].x = (LONG)(ox + h); pts[1].y = (LONG)(oy - h * 0.6f);
    pts[2].x = (LONG)ox;       pts[2].y = (LONG)(oy + h * 0.6f);
  }
  else {
    float d = (float)dir;
    pts[0].x = (LONG)(ox - d * h * 0.6f); pts[0].y = (LONG)(oy - h);
    pts[1].x = (LONG)(ox - d * h * 0.6f); pts[1].y = (LONG)(oy + h);
    pts[2].x = (LONG)(ox + d * h * 0.6f); pts[2].y = (LONG)oy;
  }
  HBRUSH brush = CreateSolidBrush(RGB(col.red, col.green, col.blue));
  HGDIOBJ old_brush = SelectObject(dc, brush), old_pen = SelectObject(dc, GetStockObject(NULL_PEN));
  Polygon(dc, pts, 3);
  SelectObject(dc, old_brush);
  SelectObject(dc, old_pen);
  DeleteObject(brush);
  if (advanced)
    SetWorldTransform(dc, &old_xf);
#endif
}

//-------------------------------------------------------------------------------------------------
void DrawGear(CDrawContext* context, float x, float y, float size, const CColor& col,
              const CColor& hole_col)
{
#ifdef _WIN32
  // like DrawArrow: in hundredths of a view unit under a scaled world transform
  HDC dc = (HDC)context->getSystemContext();
  XFORM old_xf;
  bool advanced = GetGraphicsMode(dc) == GM_ADVANCED && GetWorldTransform(dc, &old_xf);
  float k = 0.01f;
  if (advanced) {
    XFORM f = {k, 0, 0, k, 0, 0};
    ModifyWorldTransform(dc, &f, MWT_LEFTMULTIPLY);
  }
  else
    k = 1.0f;
  float ox = (x + (float)context->offset.h) / k, oy = (y + (float)context->offset.v) / k;
  float r_out = size * 0.5f / k, r_in = r_out * 0.72f, r_hole = r_out * 0.32f;

  // 8 teeth: each a flat top on the outer radius and a flat valley on the inner one
  const int teeth = 8;
  POINT pts[teeth * 4];
  for (int t = 0; t < teeth; t++) {
    float a = (float)t * 6.2831853f / (float)teeth, half = 3.1415927f / (float)teeth;
    float angs[4] = {a - half * 0.45f, a + half * 0.45f, a + half * 0.75f, a + half * 1.55f};
    float rads[4] = {r_out, r_out, r_in, r_in};
    for (int j = 0; j < 4; j++) {
      pts[t * 4 + j].x = (LONG)(ox + rads[j] * cosf(angs[j]));
      pts[t * 4 + j].y = (LONG)(oy + rads[j] * sinf(angs[j]));
    }
  }
  HBRUSH brush = CreateSolidBrush(RGB(col.red, col.green, col.blue));
  HBRUSH hole = CreateSolidBrush(RGB(hole_col.red, hole_col.green, hole_col.blue));
  HGDIOBJ old_brush = SelectObject(dc, brush), old_pen = SelectObject(dc, GetStockObject(NULL_PEN));
  Polygon(dc, pts, teeth * 4);
  SelectObject(dc, hole);
  Ellipse(dc, (int)(ox - r_hole), (int)(oy - r_hole), (int)(ox + r_hole), (int)(oy + r_hole));
  SelectObject(dc, old_brush);
  SelectObject(dc, old_pen);
  DeleteObject(brush);
  DeleteObject(hole);
  if (advanced)
    SetWorldTransform(dc, &old_xf);
#endif
}

//-------------------------------------------------------------------------------------------------
void OffsetDrawContext(CDrawContext* dc, CView* view)
// set the drawing context offset to match the given view
{
  // get the view size in view coords
  CastCRect sz(view);

  // find the frame rectangle
  RECT frame_rect;
  GetWindowRect((HWND)view->getFrame()->getSystemWindow(), &frame_rect);
  dc->offsetScreen.x = frame_rect.left;
  dc->offsetScreen.y = frame_rect.top;

  // DtBlkFix: screen position in view units when the UI is scaled
  float scale = view->getFrame()->getScale();
  if (scale != 1.0f) {
    dc->offsetScreen.x = RndToInt(frame_rect.left / scale);
    dc->offsetScreen.y = RndToInt(frame_rect.top / scale);
  }

  // convert to frame coords, make sure we use the CView conversion since it gives us the
  // result in parent coords (which is what VSTGUI calls local coords)
  CPoint p(0, 0);
  view->CView::localToFrame(p);

  // change context origin to match view coords
  dc->offsetScreen += p;
  dc->offset = p;

  // only allow drawing inside view
  // dc->setClipRect(sz);
}

//*************************************************************************************************
// CContextRGBA
//*************************************************************************************************
//-------------------------------------------------------------------------------------------------
CContextRGBA::CContextRGBA(int width, // pixels (default 0)
                           int height // pixels (default 0)
                           )
    : CDrawContext(NULL, // frame - don't need it we're always RGBA
                   NULL, // system context
                   NULL  // window
      )
{
  _hbm = NULL;
  _hbm_prev = NULL;
  _width = _height = 0;
  _scale = 1.0f;
  _pix_width = _pix_height = 0;

  HDC wnd_dc = GetDC(NULL);
  if (!wnd_dc)
    return;
  pSystemContext = CreateCompatibleDC(wnd_dc);
  ReleaseDC(NULL, wnd_dc);

  if (width && height)
    create(width, height);
}

//-------------------------------------------------------------------------------------------------
CContextRGBA::~CContextRGBA()
{
  cleanup();

  if (pSystemContext)
    DeleteDC((HDC)pSystemContext);

  pSystemContext = NULL;
}

//-------------------------------------------------------------------------------------------------
void CContextRGBA::cleanup()
{
  if (_hbm_prev) {
    SelectObject((HDC)pSystemContext, _hbm_prev);
    _hbm_prev = NULL;
  }

  if (_hbm) {
    DeleteObject(_hbm);
    _hbm = NULL;
  }
}

//-------------------------------------------------------------------------------------------------
void CContextRGBA::create(int width, int height, float scale)
// create an RGB+alpha bitmap
// expect pSystemContext to have been filled in
{
  if (!pSystemContext)
    return;

  cleanup();

  _width = width;
  _height = height;

  // DtBlkFix: bitmap is "scale" pixels per view unit & GDI drawing is scaled to match
  _scale = scale;
  _pix_width = scale == 1.0f ? width : RndToInt(width * scale);
  _pix_height = scale == 1.0f ? height : RndToInt(height * scale);
  if (scale != 1.0f) {
    XFORM xf = {scale, 0.0f, 0.0f, scale, 0.0f, 0.0f};
    SetGraphicsMode((HDC)pSystemContext, GM_ADVANCED);
    SetWorldTransform((HDC)pSystemContext, &xf);
  }
  else if (GetGraphicsMode((HDC)pSystemContext) == GM_ADVANCED)
    ModifyWorldTransform((HDC)pSystemContext, NULL, MWT_IDENTITY);

  /*CDrawContext::*/ clipRect = CRect(0, 0, width, height);

  BITMAPV5HEADER bm_info;
  memset(&bm_info, 0, sizeof(bm_info));
  bm_info.bV5Size = sizeof(BITMAPV5HEADER);
  bm_info.bV5Width = _pix_width;
  bm_info.bV5Height = _pix_height;
  bm_info.bV5Planes = 1;
  bm_info.bV5BitCount = 32;
  bm_info.bV5Compression = BI_BITFIELDS;
  bm_info.bV5RedMask = 0x00FF0000;
  bm_info.bV5GreenMask = 0x0000FF00;
  bm_info.bV5BlueMask = 0x000000FF;
  bm_info.bV5AlphaMask = 0xFF000000;

  void* data_ = NULL;
  _hbm = CreateDIBSection(
      (HDC)pSystemContext, (BITMAPINFO*)&bm_info, DIB_RGB_COLORS, &data_, NULL, 0x0);
  if (!_hbm)
    return;

  _bitmap_data = (unsigned long*)data_;
  _hbm_prev = SelectObject((HDC)pSystemContext, _hbm);

  // flush any pending drawing operations (I doubt that there would be any from us here)
  GdiFlush();
}

//*************************************************************************************************
// CopyResourceBitmapToAlphaContext
//*************************************************************************************************

//-------------------------------------------------------------------------------------------------
struct CopyResourceBitmapToAlphaContext
// use this just like you would call a function, it is a class to make cleanup easy
//
{
  HBITMAP hbm; // resource bitmap
  HDC hdc;     // temp dc

  CopyResourceBitmapToAlphaContext(CContextRGBA* ac,     // alpha context
                                   int resoure_bitmap_id // resource bitmap to copy
  )
  // create by copying from <resource_bitmap_id>
  {
    // load the bitmap resource
    extern void* hInstance; // expect this to be saved from HINSTANCE passed to DllMain
    hbm = (HBITMAP)LoadImage((HINSTANCE)hInstance,
                             MAKEINTRESOURCE(resoure_bitmap_id),
                             IMAGE_BITMAP,
                             0,
                             0,                  // default width & height
                             LR_CREATEDIBSECTION // device independent mode
    );
    hdc = GetDC(NULL);
    if (!hdc || !hbm)
      return;

    // get the size of the bitmap
    BITMAP bm_params;
    memset(&bm_params, 0, sizeof(bm_params));
    GetObject(hbm, sizeof(bm_params), &bm_params);
    if (!bm_params.bmWidth || !bm_params.bmHeight)
      return;

    // get some memory of the same size
    ac->create(bm_params.bmWidth, bm_params.bmHeight);
    if (!ac->ok())
      return;

    BITMAPV5HEADER bm_info;
    memset(&bm_info, 0, sizeof(bm_info));
    bm_info.bV5Size = sizeof(BITMAPV5HEADER);
    bm_info.bV5Width = ac->getWidth();
    bm_info.bV5Height = ac->getHeight();
    bm_info.bV5Planes = 1;
    bm_info.bV5BitCount = 32;
    bm_info.bV5Compression = BI_BITFIELDS;
    bm_info.bV5RedMask = 0x00FF0000;
    bm_info.bV5GreenMask = 0x0000FF00;
    bm_info.bV5BlueMask = 0x000000FF;
    bm_info.bV5AlphaMask = 0xFF000000;

    // copy resource bitmap to our own bitmap memory
    GetDIBits(hdc,                   //
              hbm,                   // source bitmap
              0,                     // start line
              bm_params.bmHeight,    // num lines
              ac->data(),            // destination
              (BITMAPINFO*)&bm_info, // dest bitmap info
              DIB_RGB_COLORS         //
    );
  }

  ~CopyResourceBitmapToAlphaContext()
  {
    if (hdc)
      ReleaseDC(NULL, hdc);
    if (hbm)
      DeleteObject(hbm);
  }
};

//-------------------------------------------------------------------------------------------------
void CContextRGBA::createFromResourceBitmap(int resoure_bitmap_id // resource bitmap to copy
)
{
  CopyResourceBitmapToAlphaContext(this, resoure_bitmap_id);
}

//*************************************************************************************************
// DtHSlider
//*************************************************************************************************

void GenerateGradientRGBA(
    Rng<unsigned long> dst,      // destination RGBA
    Rng<Array<float, 5>> col_vec // 5 element array contains: position (0..1), r, g, b, a
)
// generate an RGBA gradient into dst_rgba from set points in "col_vec"
// RGBA as 32 bit host-order colour
{
  // index into "col_vec"
  int col_vec_i = 0;

  // 5 element array contains: position, RGBA
  Array<float, 5> a, b;

  // interpolation fraction
  ValStp<float> frac;

  // current index range into output data
  int end_i = 0, beg_i;

  // get the RGB out of row-0 in "col_vec"
  b = col_vec[col_vec_i];

  // generate the gradient using specified anchor points
  for (int i = 0; i < dst.n; i++) {
    // chk whether we need to get the next element out of "col_vec"
    if (i >= end_i) {
      beg_i = end_i;

      a = b;

      if (col_vec_i < col_vec.n - 1)
        col_vec_i++;

      // get the next colour to interp to
      b = col_vec[col_vec_i];

      // where does this segment end?
      end_i = RndToInt(b[0] * (float)dst.n);
      if (end_i < beg_i + 1)
        end_i = beg_i + 1;

      // work out interpolation fraction step
      frac.set(/*value*/ 0.0f, /*step*/ 1.0f / (float)(end_i - beg_i));
    }

    // rgba colour
    unsigned c[4];

    // interpolate colours from a & b (note that these contain: position, r, g, b, a)
    for (int j = 0; j < 4; j++)
      c[j] = (unsigned)limit_range(lin_interp(frac, a[j + 1], b[j + 1]) * 255.0f, 0.0f, 255.0f);

    PtrRGBA(dst + i).set(/*r*/ c[0], /*g*/ c[1], /*b*/ c[2], /*a*/ c[3]);

    //
    frac.next();
  }
}

//*************************************************************************************************
// DtHSlider
//*************************************************************************************************

//-------------------------------------------------------------------------------------------------
DtHSlider::DtHSlider()
    : CControl(CRect())
{
  init();
}
DtHSlider::~DtHSlider() {}

//-------------------------------------------------------------------------------------------------
void DtHSlider::init()
{
  setListener(NULL);
  CControl::wheelInc = 0.1f;
  _step_increment = 0.005f;
  _fine_scale_frac = 1.0f / 15.0f;
  _state = INACTIVE;
  setDirty();
}

//-------------------------------------------------------------------------------------------------
void DtHSlider::setSize(
    CastCRect slide_rect, // full rectangle extends 1/2 knob_rect either side (parent coordinates)
    int knob_width        // width of knob
)
// set the slide rectangle (rectangle bounding knob centre)
{
  //
  _slide.init((float)slide_rect.getWidth());
  _slide.setOffs((float)slide_rect.left);

  //
  _knob_width = knob_width;

  // fill in window size
  CView::size = slide_rect;

  // extend size for knob width
  CView::size.left -= knob_width / 2;
  CView::size.right -= (knob_width + 1) / 2;

  // mouseable area is the same size
  CView::mouseableArea = CView::size;

  // update position
  setValue(getValue(), /*send to listener*/ false);

  // need to redraw
  setDirty();
}

//-------------------------------------------------------------------------------------------------
void DtHSlider::setState(State state, int click_pos_x /*parent coords*/, float drag_scale /*=1.0f*/)
// call to force slider into a particular state
{
  _drag.init(_slide.eff_len + 1, 1.0f / drag_scale);
  _drag.setOffs(getValue(), (float)click_pos_x);
  _state = state;
  _drag_scale = drag_scale;

  setDirty();
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult DtHSlider::onMouseDown(CPoint& mouse, const long& buttons)
// virtual, override CView
{
  // start edit
  beginEdit();

  setState(isInKnob(/*parent coords*/ mouse) ? DRAGGING : STEPPING,
           mouse.x, // click pos x
           (buttons & kLButton) && !(buttons & kControl) ? 1.0f : _fine_scale_frac);
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult DtHSlider::onMouseUp(CPoint& mouse, const long& buttons)
// virtual, override CView
{
  // release edit
  if (_state >= DRAGGING)
    endEdit();

  // treat this the same as a mouse move
  return onMouseMoved(mouse, buttons);
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult DtHSlider::onMouseMoved(CPoint& mouse, const long& buttons)
// virtual, override CView
{
  ChkChg state_chg;

  // check for button up
  if ((buttons & (kLButton | kRButton)) == 0) {
    // what is the mouse over?
    state_chg(_state) = isInKnob(mouse) ? OVER_KNOB : FOCUS;
  }

  // mouse button is pressed, check what we need to do
  else
    switch (_state) {
      case DRAGGING: {
        setValue(_drag.getFrac((float)mouse.x), /*send to listener*/ true);
        return kMouseEventHandled;
      }
      case STEPPING: {
        float delta = 0.0f;
        int knob_x = getKnobCentX();
        if (mouse.x > knob_x)
          delta = 1.0f;
        if (mouse.x < knob_x)
          delta = -1.0f;
        setValue(getValue() + _step_increment * _drag_scale * delta, /*send to listener*/ true);
        return kMouseEventHandled;
      }
      default:
        // button is pressed but we didn't get the click, make sure we're inactive
        state_chg(_state) = INACTIVE;
    }

  // mark for redraw if need be
  if (state_chg)
    setDirty();

  //
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult DtHSlider::onMouseEntered(CPoint& mouse, const long& buttons)
// virtual, override CView
{
  // treat the same as mouse moved
  return onMouseMoved(mouse, buttons);
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult DtHSlider::onMouseExited(CPoint& /*mouse*/, const long& /*buttons*/)
// virtual, override CView
{
  _state = INACTIVE;
  setDirty();
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
bool DtHSlider::onWheel(const CPoint& /*mouse*/, const CMouseWheelAxis& /*axis*/,
                        const float& distance, const long& buttons)
// virtual, override CView
{
  float delta = /*CControl::*/ wheelInc * distance * (buttons & kControl ? _fine_scale_frac : 1.0f);
  setValue(getValue() + delta, /*send to listener*/ true);
  return true;
}

//-------------------------------------------------------------------------------------------------
void DtHSlider::setValue(float /*0..1*/ v, bool send_to_listener)
//
// sets the value & control to dirty if the value changed
{
  // make sure v is inbounds
  v = limit_range(v, 0.0f, 1.0f);

  // do nothing if there was no change
  if (CControl::value == v)
    return;

  // update
  CControl::value = v;

  // we need a redraw
  setDirty();

  // send the new value to the listener if need be (due to GUI activity)
  if (send_to_listener && listener)
    listener->valueChanged(this);
}

//*************************************************************************************************
// DtPopupWdw
//*************************************************************************************************

//-------------------------------------------------------------------------------------------------
DtPopupWdw::DtPopupWdw()
{
  _wdw = NULL;
  _scale = 1.0f;
}

//-------------------------------------------------------------------------------------------------
DtPopupWdw::~DtPopupWdw()
{
  close();
}

//-------------------------------------------------------------------------------------------------
void DtPopupWdw::close()
{
  if (_wdw)
    DestroyWindow(_wdw);
  _wdw = NULL;
}

//-------------------------------------------------------------------------------------------------
bool DtPopupWdw::create(CFrame* parent_frame)
//
// attach to and use the message processing loop from parent_frame
//
{
  // ensure old one is closed
  close();

  CharArray<32> msw_class_name;

  // DtBlkFix: screen positions are in view units of the frame
  _scale = parent_frame->getScale();

  // get microsoft windows class name from the frame
  HWND msw_parent = (HWND)parent_frame->getSystemWindow();
  if (!GetClassName(msw_parent, msw_class_name, msw_class_name.size()))
    return false;

  // Create the Window (with nothing inside - rely on update() being called)
  _wdw = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST, // extended window style
                        msw_class_name,                // windows class name
                        NULL,                          // window name
                        WS_POPUP,                      // style
                        CW_USEDEFAULT,
                        CW_USEDEFAULT, // x, y
                        0,
                        0,                    // width, height
                        msw_parent,           // parent window
                        NULL,                 // menu
                        (HINSTANCE)hInstance, // DLL instance
                        NULL                  // initial param sent to the created window
  );
  if (!_wdw)
    return false;

  // todo: one day make this able to receive messages via the VST gui loop
  SetWindowLongPtr(_wdw, GWLP_USERDATA, NULL /*(LONG_PTR)parent_frame*/);

  return true;
}

//-------------------------------------------------------------------------------------------------
void DtPopupWdw::show(bool state)
// show the window but don't take focus (activate)
{
  ShowWindow(_wdw, state ? SW_SHOWNOACTIVATE : SW_HIDE);
}

//-------------------------------------------------------------------------------------------------
void DtPopupWdw::update(CastCRect sz,               // screen position & size of window
                        CContextRGBA* bm,           // source bitmap to use
                        CPoint bm_offs,             // offset into bitmap
                        unsigned char overall_alpha // overall alpha
)
{
  _size = sz;

  if (!_wdw)
    return;
  if (!bm->ok())
    return;

  BLENDFUNCTION blend_fn;
  blend_fn.BlendOp = AC_SRC_OVER;
  blend_fn.BlendFlags = 0;
  blend_fn.SourceConstantAlpha = overall_alpha;
  blend_fn.AlphaFormat = AC_SRC_ALPHA;

  POINT msw_src_pos = {bm_offs.x, bm_offs.y};
  POINT msw_top_left = {sz.x, sz.y};
  SIZE msw_wdw_sz = {sz.getWidth(), sz.getHeight()};

  // DtBlkFix: in pixels when scaled
  if (_scale != 1.0f || bm->getScale() != 1.0f) {
    msw_src_pos.x = RndToInt(bm_offs.x * bm->getScale());
    msw_src_pos.y = RndToInt(bm_offs.y * bm->getScale());
    msw_top_left.x = RndToInt(sz.x * _scale);
    msw_top_left.y = RndToInt(sz.y * _scale);
    msw_wdw_sz.cx = RndToInt(sz.getWidth() * bm->getScale());
    msw_wdw_sz.cy = RndToInt(sz.getHeight() * bm->getScale());
  }

  // perform the alpha blend
  UpdateLayeredWindow(_wdw,                        // window to update
                      NULL,                        // screen device context (for palette matching)
                      &msw_top_left,               // top left (screen coords)
                      &msw_wdw_sz,                 // window size
                      (HDC)bm->getSystemContext(), // source bitmap
                      &msw_src_pos,                // position in bitmap to copy from
                      0,                           // colour key
                      &blend_fn,
                      ULW_ALPHA);
}

//-------------------------------------------------------------------------------------------------
void DtPopupWdw::move(CPoint top_left // top-left screen position
)
{
  _size = MoveTo(top_left, _size);
  if (!_wdw)
    return;

  // DtBlkFix: in pixels when scaled
  if (_scale != 1.0f) {
    RECT r;
    GetWindowRect(_wdw, &r);
    MoveWindow(_wdw,
               RndToInt(top_left.x * _scale),
               RndToInt(top_left.y * _scale),
               r.right - r.left,
               r.bottom - r.top,
               /*repaint*/ false);
    return;
  }

  MoveWindow(_wdw, top_left.x, top_left.y, _size.getWidth(), _size.getHeight(), /*repaint*/ false);
}

//*************************************************************************************************
// DtPopupHSliderInternalInternal
//*************************************************************************************************

//-------------------------------------------------------------------------------------------------
DtPopupHSliderInternal::DtPopupHSliderInternal() {}

//-------------------------------------------------------------------------------------------------
DtPopupHSliderInternal::~DtPopupHSliderInternal() {}

//-------------------------------------------------------------------------------------------------
void DtPopupHSliderInternal::create(CFrame* frame, //
                                    int top,       // y position of top of rectangle (screen coords)
                                    int width,     // width of active area
                                    int height,    // height of active area
                                    float magnify  // magnify scaling to apply to slider
)
{
  _knob_i = -1;
  _bg_i = -1;

  _magnify = magnify;

  // set the rectangle (screen coordinates), _bg_rect.x filled in by "move"
  _bg_rect = CRect(/*left*/ 0, /*top*/ top, /*right*/ width, /*bottom*/ top + height);

  // effective width of screen rectangle
  _rescale.init((float)width, magnify);

  // create the window (but it's not yet visible)
  _bg_wdw.create(frame);
  _knob_wdw.create(frame);
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSliderInternal::shift(float value, // value 0..1 at "x"
                                   float pixel  // x pixel offset (screen coordinates)
)
// shifts internal variables, does not update window
{
  // effective start of screen rectangle
  _rescale.setOffs(value, pixel);

  // find value corresponding to left edge of rectangle
  float left_val = limit_range(value - 0.5f / _magnify, 0.0f, 1.0f - 1.0f / _magnify);

  // set the left edge of bg rectangle screen coordinates
  _bg_rect.moveTo(/*x*/ RndToInt(getPixel(left_val)), _bg_rect.y);
}

//-------------------------------------------------------------------------------------------------
float /*value*/ DtPopupHSliderInternal::drag(int pixel /*screen coords*/)
// drag returns value corresponding to new_knob_x and drags the background rectangle
// if out of bounds (does not update window)
//
{
  float pixel_f = (float)pixel;
  float value = getValue(pixel_f);

  // drag bg rectangle with mouse if the mouse moves beyond left or right edges
  if (pixel < _bg_rect.left)
    _bg_rect = MoveLeftEdge(pixel, _bg_rect);

  else if (pixel >= _bg_rect.right)
    _bg_rect = MoveRightEdge(pixel + 1, _bg_rect);

  // no need to shift bg rect
  else
    return value;

  // move rescale to match bg rect
  _rescale.setOffs(value, pixel_f);

  return value;
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSliderInternal::update(
    float value,           // position of knob (0..1)
    int bm_idx,            // which image to use from the bitmaps (assume vertically stacked)
    CContextRGBA* bg_bm,   // background bitmap
    CContextRGBA* knob_bm, // knob bitmap
    bool force_full_update)
{
  // get knob x position
  int knob_x = RndToInt(getPixel(value));

  /*
  // shift the bg rect if the knob has gone off the edge
  if(_enable_bg_shift) {
          if(knob_x < _bg_rect.left)
                  _bg_rect = MoveLeftEdge(knob_x, _bg_rect);
          else if(knob_x >= _bg_rect.right)
                  _bg_rect = MoveRightEdge(knob_x+1, _bg_rect);
  }
  */

  // centre of active background area
  CPoint bg_cent = GetCent(_bg_rect);

  //
  // update background
  //

  //
  ChkChg chg_bg = force_full_update;
  chg_bg(_bg_i) = bm_idx;

  // get background tile
  CRect bg_tile = GetTile(bg_bm, /*num x*/ 1, /*num y*/ 2, /*idx x*/ 0, /*idx y*/ _bg_i);

  // centre bitmap over active background rectangle
  CRect bg_rect = MoveCent(bg_cent, bg_tile);

  // decide whether to move the window or do a full update
  if (!chg_bg)
    _bg_wdw.move(TopLeft(bg_rect));
  else
    _bg_wdw.update(bg_rect,         // window pos & size
                   bg_bm,           // bitmap to use
                   TopLeft(bg_tile) // offset into bitmap
    );

  //
  // update knob
  //

  // determine which knob image to use
  ChkChg chg_knob = force_full_update;

  // which knob to use
  chg_knob(_knob_i) = bm_idx;

  // get tiled knob image
  CRect knob_tile = GetTile(knob_bm, /*num x*/ 1, /*num y*/ 2, /*idx x*/ 0, /*idx y*/ _knob_i);

  // find the knob centre
  CPoint knob_cent(knob_x, bg_cent.y);

  // find the knob image rectangle
  CRect knob_rect = MoveCent(knob_cent, knob_tile);

  //
  if (!chg_knob)
    _knob_wdw.move(TopLeft(knob_rect));
  else
    _knob_wdw.update(knob_rect,         // window pos & size
                     knob_bm,           // bitmap
                     TopLeft(knob_tile) // offset into bitmap
    );
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSliderInternal::show()
{
  _bg_wdw.show();
  _knob_wdw.show();
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSliderInternal::close()
{
  _knob_wdw.close();
  _bg_wdw.close();
}

//*************************************************************************************************
// DtPopupHSlider
//*************************************************************************************************

//-------------------------------------------------------------------------------------------------
DtPopupHSlider::DtPopupHSlider()
    : COptionMenu(CRect(), /*listener*/ NULL, /*tag*/ -1)
{
  //
  setMenuSnap(false);

  init();
}

//-------------------------------------------------------------------------------------------------
DtPopupHSlider::~DtPopupHSlider() {}

//-------------------------------------------------------------------------------------------------
void DtPopupHSlider::setMenuSnap(bool snap_state)
{
  _snap_to_menu = snap_state;

  // set or clear this state
  if (snap_state)
    setStyle(getStyle() | kCheckStyle);
  else
    setStyle(getStyle() & ~kCheckStyle);
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSlider::init()
{
  // initialize internals back to defaults

  // we are our own listener so that we can intercept COptionMenu changes
  CControl::listener = this;

  // no menu item selected yet
  _last_menu_idx = -1;

  //
  /*CControl::*/ wheelInc = 0.1f;

  // default popup images
  _bg_bm = Images::g_bg;
  _knob_bm = Images::g_knob;

#ifdef _DEBUG
// check that images are loaded
#endif

  _fine_scale_frac = 1.0f / 15.0f;

  // accept all buttons in the hittest
  _hittest_button_mask = -1;

  //
  _state = INACTIVE;

  //
  setDirty();
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSlider::setSize(CastCRect rect)
{
  CView::size = rect;
  CView::mouseableArea = rect;
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSlider::setValue(float v, bool send_to_listener)
// note that this is *not* the virtual setValue() - we don't mess with that one since it
// is called by "COptionMenu::takFocus()"
{
  // ensure in-bounds
  v = limit_range(v, 0.0f, 1.0f);

  // do nothing if no change
  if (v == CControl::value)
    return;

  //
  CControl::value = v;

  // notice that setDirty must come AFTER updating value to ensure that the GUI will not
  // miss any value changes if it happens to be running concurrently with this code
  setDirty();

  // send the new value to the listener if need be (due to GUI activity)
  if (send_to_listener && _listener)
    _listener->valueChanged(this);
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSlider::removeAllMenuEntries()
{
  COptionMenu::removeAllEntry();
  _menu_item_to_value.clear();
}

//-------------------------------------------------------------------------------------------------
int DtPopupHSlider::addMenuEntry(float value,    // 0..1 or -1 for no associated value
                                 const char* txt // menu text
)
{
  // value
  _menu_item_to_value.push_back(value);

  // text
  COptionMenu::addEntry(txt);

  //
  return (int)_menu_item_to_value.size() - 1;
}

//-------------------------------------------------------------------------------------------------
bool DtPopupHSlider::setMenuEntry(long index,     // menu index
                                  float value,    // 0..1 or -ve for no associated value
                                  const char* txt //
)
{
  if (!idx_within(index, _menu_item_to_value))
    return false;

  if (!COptionMenu::setEntry(index, const_cast<char*>(txt)))
    return false;

  _menu_item_to_value[index] = value;
  return true;
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSlider::updatePopups()
// update screen position & bitmaps of popup sliders
{
  // for safety
  if (!popupsAreOpen())
    return;
  if (_state < PRE_DRAG)
    return;

  // current value
  float v = getValue();

  // shift other popup
  if (_state > PRE_DRAG)
    _slider[_state == COARSE ? FINE : COARSE].shift(v, _slider[_state].getPixel(v));

  // update sliders
  for (int i = 0; i < 2; i++) {
    _slider[i].update(v,                          // position of knob
                      _state == (State)i ? 1 : 0, // image index (1=selected, 0=non selected)
                      _bg_bm[i],                  // background bitmap
                      _knob_bm[i]                 // knob bitmap
    );
  }
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult DtPopupHSlider::onMouseDown(CPoint& mouse, const long& buttons)
// virtual, override CView
{
  //
  _click_pos = mouse;

  // open the popup if it's the right button
  if ((buttons & kRButton) != 0)
    return COptionMenu::onMouseDown(mouse, buttons);

  return sliderMouseClick(mouse, buttons);
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult
DtPopupHSlider::onMouseUp(CPoint& mouse,
                          const long& buttons) ///< called when a mouse up event occurs
// virtual, override CView
{
  // make sure popups are closed
  for (int i = 0; i < 2; i++)
    _slider[i].close();

  // release edit
  if (_state > PRE_DRAG)
    endEdit();

  // should we keep focus?
  _state = mouse.isInside(size) ? FOCUS : INACTIVE;

  setDirty();
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult
DtPopupHSlider::onMouseMoved(CPoint& mouse,
                             const long& buttons) ///< called when a mouse move event occurs
// virtual, override CView
{
  ChkChg state_chg;

  // are the mouse buttons up?
  if ((buttons & (kLButton | kRButton)) == 0)
    state_chg(_state) = FOCUS;

  // button down but we're not dragging sliders
  else if (_state < PRE_DRAG)
    state_chg(_state) = INACTIVE;

  else if (_state == PRE_DRAG) {
    // check whether the mouse has been moved up or down, if so select that slider
    int y_delta = mouse.y - _click_pos.y;

    if (y_delta < 0) {
      beginEdit();
      _state = FINE;
    }
    else if (y_delta > 1) {
      beginEdit();
      _state = COARSE;
    }
  }
  // dragging
  else {
    //
    CPoint screen_mouse = mouse + _wdw_screen_offs;

    // which slider is active?
    for (int i = 0; i < 2; i++) {
      if (screen_mouse.isInside(_slider[i].getBgRect()))
        _state = (State)i;
    }

    // find the new value from the active slider
    float new_value = _slider[(int)_state].drag(screen_mouse.x);

    // snap to menu items if need be
    if (_snap_to_menu) {
      int menu_idx = findClosestMenuIdx(new_value);

      if (idx_within(menu_idx, _menu_item_to_value))
        new_value = _menu_item_to_value[menu_idx];
    }
    setValue(new_value, /*send to listener*/ true);

    // shift position of popups
    updatePopups();
  }

  // trigger redraw if need be
  if (state_chg)
    setDirty();

  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult
DtPopupHSlider::onMouseEntered(CPoint& mouse,
                               const long& buttons) ///< called when the mouse enters this view
// virtual, override CView
{
  // treat mouse entered the same as mouse move
  return onMouseMoved(mouse, buttons);
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult
DtPopupHSlider::onMouseExited(CPoint& mouse,
                              const long& buttons) ///< called when the mouse leaves this view
// virtual, override CView
{
  _state = INACTIVE;
  setDirty();
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
bool DtPopupHSlider::onWheel(
    const CPoint& where, const CMouseWheelAxis& axis, const float& distance,
    const long& buttons) ///< called if a mouse wheel event is happening over this view
// virtual, override CView
{
  float delta = CControl::wheelInc * distance * (buttons & kControl ? _fine_scale_frac : 1.0f);
  beginEdit();
  setValue(getValue() + delta, /*send to listener*/ true);
  endEdit();
  return true;
}

//-------------------------------------------------------------------------------------------------
bool DtPopupHSlider::hitTest(const CPoint& mouse, const long buttons)
// virtual, override CView
// called when there's a mouse click to determine whether we should take the click or not
{
  // check whether we are allowed to accept this
  if ((buttons & _hittest_button_mask) == 0)
    return false;

  // call down
  return CControl::hitTest(mouse, buttons);
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSlider::takeFocus()
// virtual, override COptionMenu
// called to do the menu popup and doesn't return until menu dismissed
{
  //
  _state = MENU_OPEN;

  // save this since takeFocus will change it to an index that we can't map to a value
  _value_before_take_focus = getValue();

  //
  setDirty();

  // select closest menu item
  // find the closest menu item to the current value
  if (_snap_to_menu) {
    int idx = findClosestMenuIdx(getValue());
    if (!COptionMenu::setCurrent(idx, /*count_separator*/ true))
      /*COptionMenu::*/ currentIndex = -1;
  }

  // this does not return until the menu is closed
  COptionMenu::takeFocus();

  // assume inactive for now
  _state = INACTIVE;

  // nothing is selected now
  _last_menu_idx = -1;

  //
  setDirty();

  // discard mouse click because if there is one then the menu must have been dismissed
  // without anything being selected
  DiscardUnprocessedMouseClick();

  // need to do this since the mouse move is lost
  GenerateMouseMoved(getFrame());
}

//-------------------------------------------------------------------------------------------------
void DtPopupHSlider::valueChanged(VSTGUI::CControl* pControl)
// virtual, override CControlListener
//
// COptionMenu::takeFocus will call here after a menu item has been selected
//
// expect that COptionMenu::currentIndex has been filled by the virtual setValue()
//
{
  // store the last menu index so that listener->valueChanged() called in setValue can check
  _last_menu_idx = COptionMenu::currentIndex;

  float v = -1;

  // see whether we can turn the menu index into a value
  if (idx_within(_last_menu_idx, _menu_item_to_value))
    v = _menu_item_to_value[_last_menu_idx];

  // default to the value stored prior to COptionMenu::takeFocus() if it is out
  // of range
  if (!within(v, 0.0f, 1.0f))
    v = _value_before_take_focus;

  // force the value to be updated (by setValue()) in case the listener wants to change the value
  // based on the menu index selected
  CControl::value = -1;

  // send it off
  setValue(v, /*send to listener*/ true);
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult DtPopupHSlider::sliderMouseClick(CPoint mouse, long buttons)
// open our popup sliders
{
  int i;

  // ignore mouse clicks if we're already dragging
  if (_state >= MENU_OPEN)
    return kMouseEventHandled;

  // otherwise popup our own stuff
  //
  _state = PRE_DRAG;

  // find offset from frame(0,0) to top left of our window
  _wdw_frame_offs = CPoint(0, 0);
  localToFrame(_wdw_frame_offs);

  // find offset from screen(0,0) to top left of our window
  POINT pt = {0, 0};
  ClientToScreen((HWND)getFrame()->getSystemWindow(), &pt);
  _wdw_screen_offs = _wdw_frame_offs + CPoint(pt.x, pt.y);

  // DtBlkFix: screen position in view units when the UI is scaled
  float scale = getFrame()->getScale();
  if (scale != 1.0f)
    _wdw_screen_offs = _wdw_frame_offs + CPoint(RndToInt(pt.x / scale), RndToInt(pt.y / scale));

  // assume 2 pixel border around images
  enum { IMAGE_BORDER = 2 };

  // fine slider is above and coarse slider sits below event window,
  int y[2];
  y[FINE] = /*CView::*/ size.top - _bg_bm[FINE]->getHeight() / 2 + IMAGE_BORDER;
  y[COARSE] = /*CView::*/ size.bottom - IMAGE_BORDER;

  float magnify[2];
  magnify[COARSE] = 1.0f;
  magnify[FINE] = 1.0f / _fine_scale_frac;

  // current value
  float v = getValue();

  // create popups
  for (i = 0; i < 2; i++) {
    _slider[i].create(getFrame(),                                    // parent frame
                      y[i] + _wdw_screen_offs.y,                     // top of slider
                      _bg_bm[i]->getWidth() - IMAGE_BORDER * 2,      // width
                      _bg_bm[i]->getHeight() / 2 - IMAGE_BORDER * 2, // height
                      magnify[i]                                     // magnify
    );

    // place slider
    _slider[i].shift(v, (float)(mouse.x + _wdw_screen_offs.x));
  }

  // update popup window images
  updatePopups();

  // make popups visible
  for (i = 0; i < 2; i++)
    _slider[i].show();

  //
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
int DtPopupHSlider::findClosestMenuIdx(float /*0..1*/ v) const
// find the closest menu index to value "v", or -1 if nothing found
{
  // currently closest index
  int closest_idx = -1;

  // any big number
  float closest_diff = 1e20f;

  // go through all menu items to see which is closest
  for (int i = 0; i < (int)_menu_item_to_value.size(); i++) {
    float diff = fabsf(v - _menu_item_to_value[i]);
    if (diff < closest_diff) {
      closest_idx = i;
      closest_diff = diff;
    }
  }

  //
  return closest_idx;
}

//*************************************************************************************************
// DtOwnerToggle
//*************************************************************************************************

DtOwnerToggle::DtOwnerToggle()
    : CControl(CRect())
{
  init();
}

DtOwnerToggle::~DtOwnerToggle() {}

//-------------------------------------------------------------------------------------------------
void DtOwnerToggle::init()
{
  setListener(NULL);

  _state = INACTIVE;

  setDirty();
}

//-------------------------------------------------------------------------------------------------
void DtOwnerToggle::setSize(CastCRect rect)
{
  CView::size = rect;
  CView::mouseableArea = rect;
}

//-------------------------------------------------------------------------------------------------
void DtOwnerToggle::setBoolValue(bool new_value, bool send_to_listener)
{
  // do nothing if
  if (new_value == getBoolValue())
    return;

  // convert bool to 1.0f or 0.0f
  CControl::value = new_value ? 1.0f : 0.0f;

  // control needs a redraw
  setDirty();

  // send the new value to the listener if need be (due to GUI activity)
  if (send_to_listener && listener)
    listener->valueChanged(this);
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult
DtOwnerToggle::onMouseDown(CPoint& mouse,
                           const long& buttons) ///< called when a mouse down event occurs
// virtual, override CView
{
  _state = MOUSE_DOWN_INSIDE;
  setDirty();
  return kMouseEventHandled;
}
//-------------------------------------------------------------------------------------------------
CMouseEventResult
DtOwnerToggle::onMouseUp(CPoint& mouse,
                         const long& buttons) ///< called when a mouse up event occurs
// virtual, override CView
{
  // don't do anything if we didn't get the mouse press
  if (_state <= FOCUS)
    return kMouseEventHandled;

  // check whether the mouse was released inside of our view
  if (mouse.isInside(CView::size)) {
    beginEdit();
    setBoolValue(!getBoolValue(), /*send to listener*/ true);
    endEdit();
    _state = FOCUS;
  }
  else {
    // mouse was released outside of our rectangle - which means cancel
    _state = INACTIVE;
  }

  setDirty();
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult
DtOwnerToggle::onMouseMoved(CPoint& mouse,
                            const long& buttons) ///< called when a mouse move event occurs
// virtual, override CView
{
  ChkChg state_chg;

  if (_state > FOCUS) {
    // mouse is pressed
    // if the mouse is moved outside of our rectangle while clicked then this acts as a "cancel"
    state_chg(_state) = mouse.isInside(CView::size) ? MOUSE_DOWN_INSIDE : MOUSE_DOWN_OUTSIDE;
  }
  else {
    // mouse is not pressed
    state_chg(_state) = FOCUS;
  }

  // mark for redraw if need be
  if (state_chg)
    setDirty();

  //
  return kMouseEventHandled;
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult
DtOwnerToggle::onMouseEntered(CPoint& mouse,
                              const long& buttons) ///< called when the mouse enters this view
// virtual, override CView
{
  // treat mouse entered & mouse move the same
  return onMouseMoved(mouse, buttons);
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult
DtOwnerToggle::onMouseExited(CPoint& mouse,
                             const long& buttons) ///< called when the mouse leaves this view
// virtual, override CView
{
  _state = INACTIVE;
  setDirty();
  return kMouseEventHandled;
}

//*************************************************************************************************
// COptionMenuOwnerDraw
//*************************************************************************************************

//-------------------------------------------------------------------------------------------------
COptionMenuOwnerDraw::COptionMenuOwnerDraw()
    : COptionMenu(CRect(), /*listener*/ NULL, /*tag*/ -1)
{
  init();

  // default to check style
  setStyle(kCheckStyle);
}

//-------------------------------------------------------------------------------------------------
void COptionMenuOwnerDraw::init()
{
  _state = INACTIVE;
}

//-------------------------------------------------------------------------------------------------
void COptionMenuOwnerDraw::setSize(CastCRect r)
{
  CView::size = r;
  CView::mouseableArea = r;
}
//-------------------------------------------------------------------------------------------------
CMouseEventResult
COptionMenuOwnerDraw::onMouseDown(CPoint& mouse,
                                  const long& buttons) ///< called when a mouse down event occurs
{
  _state = INACTIVE;
  setDirty();
  return COptionMenu::onMouseDown(mouse, buttons);
}
//-------------------------------------------------------------------------------------------------
CMouseEventResult COptionMenuOwnerDraw::onMouseEntered(CPoint& mouse, const long& buttons)
{
  _state = FOCUS;
  setDirty();
  return COptionMenu::onMouseEntered(mouse, buttons);
}

//-------------------------------------------------------------------------------------------------
CMouseEventResult COptionMenuOwnerDraw::onMouseExited(CPoint& mouse, const long& buttons)
{
  _state = INACTIVE;
  setDirty();
  return COptionMenu::onMouseExited(mouse, buttons);
}

//-------------------------------------------------------------------------------------------------
void COptionMenuOwnerDraw::takeFocus()
// open the menu and don't return until something is selected or menu is dismissed
{
  _state = MENU_OPEN;
  setDirty();

  // open the menu and don't return until something is selected or menu is dismissed
  COptionMenu::takeFocus();

  // assume that we are inactive - the mouse move will fix this if we are actually active
  _state = INACTIVE;
  setDirty();

  // get rid of pending mouse click because there will be one if the menu was dismissed
  // without a selection being made
  DiscardUnprocessedMouseClick();

  // need to do this since the mouse move is lost
  GenerateMouseMoved(getFrame());
}

//*************************************************************************************************
// COptionMenuOwnerDraw
//*************************************************************************************************
#if 0
//-------------------------------------------------------------------------------------------------
CMouseEventResult ImprovedCFrame::onMouseDown (CPoint &mouse, const long& buttons)
{
	return CFrame::onMouseDown(mouse, buttons);

}

CMouseEventResult ImprovedCFrame::onMouseUp (CPoint &mouse, const long& buttons)
{
	return  CFrame::onMouseUp(mouse, buttons);
}

CMouseEventResult ImprovedCFrame::onMouseMoved (CPoint &mouse, const long& buttons)
{
	if(IsInside(this, mouse)) {
		
	}
	return CFrame::onMouseMoved(mouse, buttons);

}
#endif
