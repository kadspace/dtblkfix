#pragma once

#include "windows.h"
#undef fopen

#include <map>
#include <string>
#include <vector>


namespace softwin {

enum ObjKind { kPen = OBJ_PEN, kBrush = OBJ_BRUSH, kDCObj = OBJ_DC, kFont = OBJ_FONT, kBitmap = OBJ_BITMAP, kRegion = OBJ_REGION };

struct Obj {
  int kind;
  bool stock = false;
  explicit Obj(int k) : kind(k) {}
  virtual ~Obj() {}
};

struct Pen : Obj {
  int style = PS_SOLID;
  int width = 0;
  COLORREF color = 0;
  Pen() : Obj(kPen) {}
};

struct Brush : Obj {
  int style = BS_SOLID;
  COLORREF color = 0;
  Brush() : Obj(kBrush) {}
};

struct Font : Obj {
  LOGFONTA lf;
  Font() : Obj(kFont) { memset(&lf, 0, sizeof(lf)); }
};

struct Bitmap : Obj {
  int w = 0, h = 0, bpp = 32;
  int stride = 0;
  uint8_t* bits = nullptr;
  bool own = true;
  bool bottom_up = false;
  bool dib = false;
  Bitmap() : Obj(kBitmap) {}
  ~Bitmap() { if (own) free(bits); }
  uint32_t* row32(int y) { return (uint32_t*)(bits + (size_t)stride * (bottom_up ? (h - 1 - y) : y)); }
  uint8_t* row8(int y) { return bits + (size_t)stride * (bottom_up ? (h - 1 - y) : y); }
  uint32_t get(int x, int y)
  {
    if (bpp == 1)
      return row8(y)[x] ? 0x00ffffffu : 0u;
    return row32(y)[x];
  }
  void set(int x, int y, uint32_t v)
  {
    if (bpp == 1)
      row8(y)[x] = (v & 0x00ffffffu) == 0x00ffffffu ? 1 : 0;
    else
      row32(y)[x] = v;
  }
};

struct Region : Obj {
  std::vector<RECT> rects;
  Region() : Obj(kRegion) {}
};

struct Wnd;

struct DCState {
  Pen* pen;
  Brush* brush;
  Font* font;
  Bitmap* bmp;
  COLORREF text, bk;
  int bk_mode, rop2, stretch_mode, gmode, poly_fill;
  UINT text_align;
  XFORM xf;
  POINT pos, brush_org, vp_org, win_org;
  bool has_clip;
  std::vector<RECT> clip;
};

struct DC : Obj {
  Wnd* wnd = nullptr;
  bool window_dc = false;
  int ox = 0, oy = 0;
  RECT wnd_area = {0, 0, 0, 0};
  bool painting = false;
  RECT paint_rect = {0, 0, 0, 0};
  Bitmap* default_bmp = nullptr;
  DCState s;
  std::vector<DCState> saved;
  DC();
  ~DC();
  Bitmap* surface();
};

struct MenuItem {
  UINT flags;
  UINT_PTR id;
  std::string text;
};

struct Menu {
  std::vector<MenuItem> items;
};

struct Wnd {
  std::string cls;
  WNDPROC proc = nullptr;
  Wnd* parent = nullptr;
  std::vector<Wnd*> children;
  RECT rc = {0, 0, 0, 0};
  LONG style = 0, exstyle = 0;
  LONG_PTR userdata = 0;
  LONG_PTR id = 0;
  HINSTANCE inst = nullptr;
  std::map<std::string, HANDLE> props;
  std::string text;
  bool visible = false;
  bool destroyed = false;
  Bitmap* surface = nullptr;
  bool has_invalid = false;
  bool erase = false;
  RECT invalid = {0, 0, 0, 0};
  HFONT font = nullptr;
  int limit = 0;
  bool edit_open = false;
  bool track_leave = false;
  HBRUSH background = nullptr;
  std::vector<uint32_t> overlay;
  POINT overlay_pos = {0, 0};
  SIZE overlay_size = {0, 0};
  bool overlay_shown = false;
  bool has_region = false;
  std::vector<RECT> region;
};

Wnd* W(HWND h);
Wnd* rootOf(Wnd* w);
POINT offsetInRoot(Wnd* w);
Host* host();
bool isEdit(Wnd* w);
void invalidateRoot(Wnd* w, const RECT& client_rect);

DC* D(HDC h);
uint32_t toPixel(COLORREF c);
COLORREF fromPixel(uint32_t p);
void devPoint(DC* dc, double x, double y, double& dx, double& dy);
double devScale(DC* dc);
void fillSpan(DC* dc, int sy, int sx0, int sx1, uint32_t pixel);
void blendSpanCoverage(DC* dc, int sy, int sx0, const uint8_t* coverage, int n, uint32_t pixel);
void touched(DC* dc, int x0, int y0, int x1, int y1);
void beginPrimitive(DC* dc);
HDC screenDC();

int textExtent(DC* dc, const char* s, int n, int* ascent, int* descent);
void drawTextRun(DC* dc, double dev_x, double dev_baseline_y, const char* s, int n, COLORREF color);
int deviceFontPixels(DC* dc, int* ascent, int* descent, int* em);

std::string unixPath(const char* p);

}
