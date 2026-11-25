#include "softwin_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>

namespace softwin {

static Pen g_stock_pens[3];
static Brush g_stock_brushes[6];
static Font g_stock_font;
static Bitmap g_screen_bitmap;
static bool g_stock_init = false;

static void initStock()
{
  if (g_stock_init)
    return;
  g_stock_init = true;
  COLORREF bc[5] = {RGB(255, 255, 255), RGB(192, 192, 192), RGB(128, 128, 128), RGB(64, 64, 64), RGB(0, 0, 0)};
  for (int i = 0; i < 5; i++) {
    g_stock_brushes[i].color = bc[i];
    g_stock_brushes[i].stock = true;
  }
  g_stock_brushes[5].style = BS_NULL;
  g_stock_brushes[5].stock = true;
  g_stock_pens[0].color = RGB(255, 255, 255);
  g_stock_pens[1].color = RGB(0, 0, 0);
  g_stock_pens[2].style = PS_NULL;
  for (auto& p : g_stock_pens)
    p.stock = true;
  g_stock_font.stock = true;
  g_stock_font.lf.lfHeight = -12;
  strcpy(g_stock_font.lf.lfFaceName, "Arial");
  g_stock_font.lf.lfWeight = FW_NORMAL;
  g_screen_bitmap.stock = true;
  g_screen_bitmap.w = 1;
  g_screen_bitmap.h = 1;
  g_screen_bitmap.stride = 4;
  g_screen_bitmap.bits = (uint8_t*)calloc(1, 4);
}

DC::DC() : Obj(kDCObj)
{
  initStock();
  default_bmp = new Bitmap;
  default_bmp->bpp = 1;
  default_bmp->w = default_bmp->h = 1;
  default_bmp->stride = 1;
  default_bmp->bits = (uint8_t*)calloc(1, 1);
  s.pen = &g_stock_pens[1];
  s.brush = &g_stock_brushes[0];
  s.font = &g_stock_font;
  s.bmp = default_bmp;
  s.text = RGB(0, 0, 0);
  s.bk = RGB(255, 255, 255);
  s.bk_mode = OPAQUE;
  s.rop2 = R2_COPYPEN;
  s.stretch_mode = BLACKONWHITE;
  s.gmode = GM_COMPATIBLE;
  s.poly_fill = ALTERNATE;
  s.text_align = TA_LEFT | TA_TOP;
  s.xf = {1, 0, 0, 1, 0, 0};
  s.pos = {0, 0};
  s.brush_org = {0, 0};
  s.vp_org = {0, 0};
  s.win_org = {0, 0};
  s.has_clip = false;
}

DC::~DC()
{
  delete default_bmp;
}

Bitmap* DC::surface()
{
  return s.bmp;
}

DC* D(HDC h)
{
  if (h)
    return (DC*)h;
  static DC* null_dc = nullptr;
  if (!null_dc) {
    null_dc = new DC;
    null_dc->s.bmp = nullptr;
  }
  null_dc->s.bmp = nullptr;
  return null_dc;
}

uint32_t toPixel(COLORREF c)
{
  return ((uint32_t)GetRValue(c) << 16) | ((uint32_t)GetGValue(c) << 8) | (uint32_t)GetBValue(c);
}

COLORREF fromPixel(uint32_t p)
{
  return RGB((p >> 16) & 0xff, (p >> 8) & 0xff, p & 0xff);
}

void devPoint(DC* dc, double x, double y, double& dx, double& dy)
{
  if (dc->s.gmode == GM_ADVANCED) {
    const XFORM& f = dc->s.xf;
    dx = x * f.eM11 + y * f.eM21 + f.eDx;
    dy = x * f.eM12 + y * f.eM22 + f.eDy;
  }
  else {
    dx = x;
    dy = y;
  }
  dx += dc->s.vp_org.x - dc->s.win_org.x;
  dy += dc->s.vp_org.y - dc->s.win_org.y;
}

double devScale(DC* dc)
{
  if (dc->s.gmode != GM_ADVANCED)
    return 1.0;
  const XFORM& f = dc->s.xf;
  return std::sqrt(std::fabs((double)f.eM11 * f.eM22 - (double)f.eM12 * f.eM21));
}

static int rnd(double v)
{
  return (int)std::floor(v + 0.5);
}

static void effClips(DC* dc, std::vector<RECT>& out)
{
  out.clear();
  Bitmap* b = dc->surface();
  if (!b || b == &g_screen_bitmap)
    return;
  RECT base = {0, 0, b->w, b->h};
  if (dc->window_dc) {
    base.left = std::max(base.left, dc->wnd_area.left);
    base.top = std::max(base.top, dc->wnd_area.top);
    base.right = std::min(base.right, dc->wnd_area.right);
    base.bottom = std::min(base.bottom, dc->wnd_area.bottom);
  }
  if (dc->painting) {
    base.left = std::max(base.left, dc->paint_rect.left + dc->ox);
    base.top = std::max(base.top, dc->paint_rect.top + dc->oy);
    base.right = std::min(base.right, dc->paint_rect.right + dc->ox);
    base.bottom = std::min(base.bottom, dc->paint_rect.bottom + dc->oy);
  }
  if (base.left >= base.right || base.top >= base.bottom)
    return;
  if (!dc->s.has_clip) {
    out.push_back(base);
    return;
  }
  for (const RECT& r : dc->s.clip) {
    RECT c = {std::max(base.left, r.left + dc->ox),
              std::max(base.top, r.top + dc->oy),
              std::min(base.right, r.right + dc->ox),
              std::min(base.bottom, r.bottom + dc->oy)};
    if (c.left < c.right && c.top < c.bottom)
      out.push_back(c);
  }
}

static thread_local std::vector<RECT> t_clips;
static thread_local DC* t_clip_dc = nullptr;

static const std::vector<RECT>& clipsFor(DC* dc)
{
  effClips(dc, t_clips);
  t_clip_dc = dc;
  return t_clips;
}

static uint32_t applyRop2(int rop2, uint32_t dst, uint32_t pen)
{
  switch (rop2) {
    case R2_XORPEN: return (dst ^ pen) & 0x00ffffffu;
    case R2_NOT: return (~dst) & 0x00ffffffu;
    case R2_NOP: return dst;
    case R2_BLACK: return 0;
    case R2_WHITE: return 0x00ffffffu;
    default: return pen;
  }
}

void touched(DC* dc, int x0, int y0, int x1, int y1)
{
  if (!dc->window_dc || !dc->wnd || dc->painting)
    return;
  Wnd* root = rootOf(dc->wnd);
  if (!root || !host())
    return;
  host()->invalidate((void*)root, SwRect{x0, y0, x1, y1});
}

static void spanRaw(DC* dc, const std::vector<RECT>& clips, int sy, int sx0, int sx1, uint32_t pixel)
{
  Bitmap* b = dc->surface();
  for (const RECT& c : clips) {
    if (sy < c.top || sy >= c.bottom)
      continue;
    int a = std::max(sx0, (int)c.left), e = std::min(sx1, (int)c.right);
    if (a >= e)
      continue;
    if (b->bpp == 1) {
      for (int x = a; x < e; x++)
        b->set(x, sy, applyRop2(dc->s.rop2, b->get(x, sy), pixel));
      continue;
    }
    uint32_t* row = b->row32(sy);
    if (dc->s.rop2 == R2_COPYPEN) {
      for (int x = a; x < e; x++)
        row[x] = pixel;
    }
    else {
      for (int x = a; x < e; x++)
        row[x] = applyRop2(dc->s.rop2, row[x], pixel);
    }
  }
}

void fillSpan(DC* dc, int sy, int sx0, int sx1, uint32_t pixel)
{
  if (sx0 >= sx1)
    return;
  spanRaw(dc, clipsFor(dc), sy, sx0, sx1, pixel);
}

void blendSpanCoverage(DC* dc, int sy, int sx0, const uint8_t* cov, int n, uint32_t pixel)
{
  Bitmap* b = dc->surface();
  if (!b || b->bpp != 32)
    return;
  const std::vector<RECT>& clips = (t_clip_dc == dc) ? t_clips : clipsFor(dc);
  uint32_t pr = (pixel >> 16) & 0xff, pg = (pixel >> 8) & 0xff, pb = pixel & 0xff;
  for (const RECT& c : clips) {
    if (sy < c.top || sy >= c.bottom)
      continue;
    int a = std::max(sx0, (int)c.left), e = std::min(sx0 + n, (int)c.right);
    uint32_t* row = b->row32(sy);
    for (int x = a; x < e; x++) {
      uint32_t k = cov[x - sx0];
      if (!k)
        continue;
      uint32_t d = row[x];
      uint32_t dr = (d >> 16) & 0xff, dg = (d >> 8) & 0xff, db = d & 0xff;
      dr = (pr * k + dr * (255 - k) + 127) / 255;
      dg = (pg * k + dg * (255 - k) + 127) / 255;
      db = (pb * k + db * (255 - k) + 127) / 255;
      row[x] = (dr << 16) | (dg << 8) | db;
    }
  }
}

void beginPrimitive(DC* dc)
{
  clipsFor(dc);
}

static void plot(DC* dc, int sx, int sy, uint32_t pixel)
{
  spanRaw(dc, t_clips, sy, sx, sx + 1, pixel);
}

static void fillPolygonDev(DC* dc, const std::vector<double>& xs, const std::vector<double>& ys,
                           const std::vector<int>& counts, uint32_t pixel, int fill_mode)
{
  if (xs.empty())
    return;
  double miny = 1e30, maxy = -1e30;
  for (double y : ys) {
    miny = std::min(miny, y);
    maxy = std::max(maxy, y);
  }
  int y0 = (int)std::ceil(miny - 0.5), y1 = (int)std::ceil(maxy - 0.5);
  struct Edge { double x0, y0, x1, y1; int dir; };
  std::vector<Edge> edges;
  size_t base = 0;
  for (int cnt : counts) {
    for (int i = 0; i < cnt; i++) {
      int j = (i + 1) % cnt;
      double ax = xs[base + i], ay = ys[base + i], bx = xs[base + j], by = ys[base + j];
      if (ay == by)
        continue;
      if (ay < by)
        edges.push_back({ax, ay, bx, by, 1});
      else
        edges.push_back({bx, by, ax, ay, -1});
    }
    base += cnt;
  }
  std::vector<std::pair<double, int>> hits;
  int ox = dc->ox, oy = dc->oy;
  int minx_all = 0, maxx_all = 0;
  bool any = false;
  for (int y = y0; y < y1; y++) {
    double cy = y + 0.5;
    hits.clear();
    for (const Edge& e : edges) {
      if (cy < e.y0 || cy >= e.y1)
        continue;
      double t = (cy - e.y0) / (e.y1 - e.y0);
      hits.push_back({e.x0 + t * (e.x1 - e.x0), e.dir});
    }
    std::sort(hits.begin(), hits.end());
    int wind = 0;
    for (size_t k = 0; k + 1 < hits.size() + 1 && k < hits.size(); k++) {
      bool inside_before = fill_mode == WINDING ? wind != 0 : (k % 2) == 1;
      wind += hits[k].second;
      bool inside_after = fill_mode == WINDING ? wind != 0 : (k % 2) == 0;
      if (!inside_before && inside_after && k + 1 < hits.size()) {
        size_t m = k + 1;
        if (fill_mode == WINDING) {
          int w2 = wind;
          while (m < hits.size()) {
            w2 += hits[m].second;
            if (w2 == 0)
              break;
            m++;
          }
          if (m >= hits.size())
            m = hits.size() - 1;
        }
        int xa = (int)std::ceil(hits[k].first - 0.5), xb = (int)std::ceil(hits[m].first - 0.5);
        if (xa < xb) {
          spanRaw(dc, t_clips, y + oy, xa + ox, xb + ox, pixel);
          if (!any) {
            minx_all = xa;
            maxx_all = xb;
            any = true;
          }
          minx_all = std::min(minx_all, xa);
          maxx_all = std::max(maxx_all, xb);
        }
      }
    }
  }
  if (any)
    touched(dc, minx_all + ox, y0 + oy, maxx_all + ox, y1 + oy);
}

static void lineDevThin(DC* dc, int x0, int y0, int x1, int y1, uint32_t pixel, bool last, int style)
{
  int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  int ox = dc->ox, oy = dc->oy;
  int step = 0;
  for (;;) {
    bool at_end = x0 == x1 && y0 == y1;
    if (at_end && !last)
      break;
    bool on = true;
    if (style == PS_DOT || style == PS_ALTERNATE)
      on = (step % 2) == 0;
    else if (style == PS_DASH)
      on = (step % 9) < 6;
    else if (style == PS_DASHDOT)
      on = (step % 12) < 6 || (step % 12) == 9;
    if (on)
      plot(dc, x0 + ox, y0 + oy, pixel);
    step++;
    if (at_end)
      break;
    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

static int penPixels(DC* dc)
{
  Pen* p = dc->s.pen;
  if (!p || (p->style & PS_STYLE_MASK) == PS_NULL)
    return 0;
  if (p->width <= 1 && !(p->style & PS_GEOMETRIC))
    return p->width == 0 ? 1 : std::max(1, rnd(devScale(dc)));
  return std::max(1, rnd(p->width * devScale(dc)));
}

static void strokeDev(DC* dc, const std::vector<double>& xs, const std::vector<double>& ys, bool closed)
{
  Pen* p = dc->s.pen;
  int w = penPixels(dc);
  if (!w || xs.size() < 1)
    return;
  uint32_t pixel = toPixel(p->color);
  int n = (int)xs.size();
  int segs = closed ? n : n - 1;
  int minx = 1 << 30, miny = 1 << 30, maxx = -(1 << 30), maxy = -(1 << 30);
  for (int i = 0; i < n; i++) {
    minx = std::min(minx, (int)std::floor(xs[i]) - w);
    maxx = std::max(maxx, (int)std::ceil(xs[i]) + w);
    miny = std::min(miny, (int)std::floor(ys[i]) - w);
    maxy = std::max(maxy, (int)std::ceil(ys[i]) + w);
  }
  if (w == 1) {
    for (int i = 0; i < segs; i++) {
      int j = (i + 1) % n;
      bool last = closed ? false : (i == segs - 1 ? false : false);
      lineDevThin(dc, rnd(xs[i]), rnd(ys[i]), rnd(xs[j]), rnd(ys[j]), pixel, last, p->style & PS_STYLE_MASK);
    }
  }
  else {
    double hw = w / 2.0;
    for (int i = 0; i < segs; i++) {
      int j = (i + 1) % n;
      double ax = xs[i], ay = ys[i], bx = xs[j], by = ys[j];
      double len = std::hypot(bx - ax, by - ay);
      if (len > 0) {
        double nx = -(by - ay) / len * hw, ny = (bx - ax) / len * hw;
        std::vector<double> px = {ax + nx, bx + nx, bx - nx, ax - nx};
        std::vector<double> py = {ay + ny, by + ny, by - ny, ay - ny};
        fillPolygonDev(dc, px, py, {4}, pixel, WINDING);
      }
      int seg = 12;
      std::vector<double> cx, cy;
      for (int k = 0; k < seg; k++) {
        double a = k * 6.283185307179586 / seg;
        cx.push_back(ax + std::cos(a) * hw);
        cy.push_back(ay + std::sin(a) * hw);
      }
      fillPolygonDev(dc, cx, cy, {seg}, pixel, WINDING);
      if (!closed && i == segs - 1) {
        for (int k = 0; k < seg; k++) {
          cx[k] = bx + std::cos(k * 6.283185307179586 / seg) * hw;
          cy[k] = by + std::sin(k * 6.283185307179586 / seg) * hw;
        }
        fillPolygonDev(dc, cx, cy, {seg}, pixel, WINDING);
      }
    }
  }
  touched(dc, minx + dc->ox, miny + dc->oy, maxx + dc->ox, maxy + dc->oy);
}

static bool brushOn(DC* dc)
{
  return dc->s.brush && dc->s.brush->style != BS_NULL;
}

static void toDev(DC* dc, const POINT* pts, int n, std::vector<double>& xs, std::vector<double>& ys)
{
  xs.resize(n);
  ys.resize(n);
  for (int i = 0; i < n; i++)
    devPoint(dc, pts[i].x, pts[i].y, xs[i], ys[i]);
}

static void ellipsePoints(double x0, double y0, double x1, double y1, double a0, double a1,
                          std::vector<double>& xs, std::vector<double>& ys)
{
  double cx = (x0 + x1) / 2.0, cy = (y0 + y1) / 2.0;
  double rx = (x1 - x0) / 2.0, ry = (y1 - y0) / 2.0;
  double span = a1 - a0;
  int n = std::max(8, (int)(std::fabs(span) * std::max(rx, ry) / 1.5));
  n = std::min(n, 2000);
  for (int i = 0; i <= n; i++) {
    double a = a0 + span * i / n;
    xs.push_back(cx + std::cos(a) * rx);
    ys.push_back(cy - std::sin(a) * ry);
  }
}

static void devBox(DC* dc, int l, int t, int r, int b, double& x0, double& y0, double& x1, double& y1)
{
  double ax, ay, bx, by;
  devPoint(dc, l, t, ax, ay);
  devPoint(dc, r, b, bx, by);
  x0 = std::min(ax, bx);
  x1 = std::max(ax, bx);
  y0 = std::min(ay, by);
  y1 = std::max(ay, by);
}

}

using namespace softwin;

HGDIOBJ WINAPI GetStockObject(int i)
{
  initStock();
  switch (i) {
    case WHITE_BRUSH: return &g_stock_brushes[0];
    case LTGRAY_BRUSH: return &g_stock_brushes[1];
    case GRAY_BRUSH: return &g_stock_brushes[2];
    case DKGRAY_BRUSH: return &g_stock_brushes[3];
    case BLACK_BRUSH: return &g_stock_brushes[4];
    case NULL_BRUSH: return &g_stock_brushes[5];
    case WHITE_PEN: return &g_stock_pens[0];
    case BLACK_PEN: return &g_stock_pens[1];
    case NULL_PEN: return &g_stock_pens[2];
    case DC_BRUSH: return &g_stock_brushes[0];
    case DC_PEN: return &g_stock_pens[1];
    default: return &g_stock_font;
  }
}

HDC softwin::screenDC()
{
  DC* dc = new DC;
  dc->s.bmp = &g_screen_bitmap;
  return (HDC)dc;
}

HDC WINAPI CreateCompatibleDC(HDC)
{
  return (HDC) new DC;
}

BOOL WINAPI DeleteDC(HDC h)
{
  DC* dc = h ? D(h) : nullptr;
  if (!dc || dc->window_dc)
    return FALSE;
  delete dc;
  return TRUE;
}

int WINAPI SaveDC(HDC h)
{
  DC* dc = D(h);
  dc->saved.push_back(dc->s);
  return (int)dc->saved.size();
}

BOOL WINAPI RestoreDC(HDC h, int n)
{
  DC* dc = D(h);
  int idx = n < 0 ? (int)dc->saved.size() + n : n - 1;
  if (idx < 0 || idx >= (int)dc->saved.size())
    return FALSE;
  dc->s = dc->saved[idx];
  dc->saved.resize(idx);
  return TRUE;
}

HGDIOBJ WINAPI SelectObject(HDC h, HGDIOBJ o)
{
  DC* dc = h ? D(h) : nullptr;
  Obj* obj = (Obj*)o;
  if (!dc || !obj)
    return NULL;
  switch (obj->kind) {
    case kPen: { HGDIOBJ prev = dc->s.pen; dc->s.pen = (Pen*)obj; return prev; }
    case kBrush: { HGDIOBJ prev = dc->s.brush; dc->s.brush = (Brush*)obj; return prev; }
    case kFont: { HGDIOBJ prev = dc->s.font; dc->s.font = (Font*)obj; return prev; }
    case kBitmap: {
      if (dc->window_dc)
        return NULL;
      HGDIOBJ prev = dc->s.bmp;
      dc->s.bmp = (Bitmap*)obj;
      return prev;
    }
    case kRegion: return (HGDIOBJ)(intptr_t)SelectClipRgn(h, (HRGN)o);
  }
  return NULL;
}

BOOL WINAPI DeleteObject(HGDIOBJ o)
{
  Obj* obj = (Obj*)o;
  if (!obj || obj->stock)
    return FALSE;
  delete obj;
  return TRUE;
}

HGDIOBJ WINAPI GetCurrentObject(HDC h, UINT type)
{
  DC* dc = D(h);
  switch (type) {
    case OBJ_PEN: return dc->s.pen;
    case OBJ_BRUSH: return dc->s.brush;
    case OBJ_FONT: return dc->s.font;
    case OBJ_BITMAP: return dc->s.bmp;
  }
  return NULL;
}

DWORD WINAPI GetObjectType(HGDIOBJ o)
{
  return o ? (DWORD)((Obj*)o)->kind : 0;
}

int WINAPI GetObjectA(HANDLE o, int size, LPVOID out)
{
  Obj* obj = (Obj*)o;
  if (!obj || !out)
    return 0;
  if (obj->kind == kBitmap && size >= (int)sizeof(BITMAP)) {
    Bitmap* b = (Bitmap*)obj;
    BITMAP bm;
    memset(&bm, 0, sizeof(bm));
    bm.bmWidth = b->w;
    bm.bmHeight = b->h;
    bm.bmPlanes = 1;
    bm.bmBitsPixel = (WORD)b->bpp;
    bm.bmWidthBytes = b->bpp == 1 ? ((b->w + 15) / 16) * 2 : b->w * 4;
    bm.bmBits = b->dib ? b->bits : nullptr;
    memcpy(out, &bm, sizeof(bm));
    return sizeof(bm);
  }
  if (obj->kind == kFont && size >= (int)sizeof(LOGFONTA)) {
    memcpy(out, &((Font*)obj)->lf, sizeof(LOGFONTA));
    return sizeof(LOGFONTA);
  }
  if (obj->kind == kPen && size >= (int)sizeof(LOGPEN)) {
    Pen* p = (Pen*)obj;
    LOGPEN lp = {(UINT)p->style, {p->width, 0}, p->color};
    memcpy(out, &lp, sizeof(lp));
    return sizeof(lp);
  }
  if (obj->kind == kBrush && size >= (int)sizeof(LOGBRUSH)) {
    Brush* b = (Brush*)obj;
    LOGBRUSH lb = {(UINT)b->style, b->color, 0};
    memcpy(out, &lb, sizeof(lb));
    return sizeof(lb);
  }
  return 0;
}

static Bitmap* newBitmap(int w, int h, int bpp)
{
  Bitmap* b = new Bitmap;
  b->w = std::max(w, 0);
  b->h = std::max(h, 0);
  b->bpp = bpp;
  b->stride = bpp == 1 ? std::max(b->w, 1) : std::max(b->w, 1) * 4;
  b->bits = (uint8_t*)calloc((size_t)b->stride * std::max(b->h, 1), 1);
  return b;
}

HBITMAP WINAPI CreateCompatibleBitmap(HDC h, int w, int ht)
{
  DC* dc = D(h);
  int bpp = 32;
  if (dc && !dc->window_dc && dc->s.bmp && dc->s.bmp->bpp == 1 && dc->s.bmp != &g_screen_bitmap)
    bpp = 1;
  return newBitmap(w, ht, bpp);
}

HBITMAP WINAPI CreateBitmap(int w, int h, UINT, UINT bpp, const void* bits)
{
  Bitmap* b = newBitmap(w, h, bpp == 1 ? 1 : 32);
  if (bits) {
    if (bpp == 1) {
      int src_stride = ((w + 15) / 16) * 2;
      for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
          b->row8(y)[x] = (((const uint8_t*)bits)[y * src_stride + x / 8] >> (7 - x % 8)) & 1;
    }
    else if (bpp == 32) {
      memcpy(b->bits, bits, (size_t)w * h * 4);
    }
  }
  return b;
}

HBITMAP WINAPI CreateDIBSection(HDC, const BITMAPINFO* bi, UINT, void** bits, HANDLE, DWORD)
{
  if (!bi)
    return NULL;
  const BITMAPINFOHEADER& h = bi->bmiHeader;
  int w = h.biWidth, ht = std::abs(h.biHeight);
  Bitmap* b = new Bitmap;
  b->w = w;
  b->h = ht;
  b->bottom_up = h.biHeight > 0;
  b->dib = true;
  if (h.biBitCount == 32) {
    b->bpp = 32;
    b->stride = w * 4;
  }
  else if (h.biBitCount == 24) {
    b->bpp = 32;
    b->stride = w * 4;
  }
  else {
    b->bpp = 32;
    b->stride = w * 4;
  }
  b->bits = (uint8_t*)calloc((size_t)b->stride * std::max(ht, 1), 1);
  if (bits)
    *bits = b->bits;
  return b;
}

int WINAPI GetDIBits(HDC, HBITMAP hb, UINT start, UINT lines, LPVOID out, LPBITMAPINFO bi, UINT)
{
  Bitmap* b = (Bitmap*)hb;
  if (!b || !bi)
    return 0;
  BITMAPINFOHEADER& h = bi->bmiHeader;
  if (!out) {
    h.biWidth = b->w;
    h.biHeight = b->h;
    h.biPlanes = 1;
    h.biBitCount = 32;
    h.biCompression = BI_RGB;
    h.biSizeImage = b->w * b->h * 4;
    return b->h;
  }
  bool bottom_up = h.biHeight > 0;
  int n = 0;
  for (UINT i = 0; i < lines && (int)(start + i) < b->h; i++, n++) {
    int y = (int)(start + i);
    int src_y = bottom_up ? b->h - 1 - y : y;
    uint32_t* dst = (uint32_t*)out + (size_t)i * b->w;
    for (int x = 0; x < b->w; x++)
      dst[x] = b->get(x, src_y);
  }
  return n;
}

int WINAPI SetDIBits(HDC, HBITMAP hb, UINT start, UINT lines, const void* in, const BITMAPINFO* bi, UINT)
{
  Bitmap* b = (Bitmap*)hb;
  if (!b || !bi || !in || bi->bmiHeader.biBitCount != 32)
    return 0;
  bool bottom_up = bi->bmiHeader.biHeight > 0;
  int n = 0;
  for (UINT i = 0; i < lines && (int)(start + i) < b->h; i++, n++) {
    int y = (int)(start + i);
    int dst_y = bottom_up ? b->h - 1 - y : y;
    const uint32_t* src = (const uint32_t*)in + (size_t)i * b->w;
    for (int x = 0; x < b->w; x++)
      b->set(x, dst_y, src[x]);
  }
  return n;
}

LONG WINAPI GetBitmapBits(HBITMAP hb, LONG n, LPVOID out)
{
  Bitmap* b = (Bitmap*)hb;
  if (!b || !out)
    return 0;
  LONG total = std::min<LONG>(n, b->w * b->h * 4);
  for (int y = 0, k = 0; y < b->h; y++)
    for (int x = 0; x < b->w && k + 4 <= total; x++, k += 4) {
      uint32_t v = b->get(x, y);
      memcpy((uint8_t*)out + k, &v, 4);
    }
  return total;
}

HBITMAP WINAPI LoadBitmapA(HINSTANCE, LPCSTR)
{
  return NULL;
}

HANDLE WINAPI LoadImageA(HINSTANCE, LPCSTR, UINT, int, int, UINT)
{
  return NULL;
}

HPEN WINAPI CreatePen(int style, int width, COLORREF c)
{
  Pen* p = new Pen;
  p->style = style;
  p->width = width;
  p->color = c;
  return p;
}

HPEN WINAPI CreatePenIndirect(const LOGPEN* lp)
{
  return CreatePen(lp->lopnStyle, lp->lopnWidth.x, lp->lopnColor);
}

HPEN WINAPI ExtCreatePen(DWORD style, DWORD width, const LOGBRUSH* lb, DWORD, const DWORD*)
{
  return CreatePen((int)style, (int)width, lb ? lb->lbColor : 0);
}

HBRUSH WINAPI CreateSolidBrush(COLORREF c)
{
  Brush* b = new Brush;
  b->color = c;
  return b;
}

HBRUSH WINAPI CreateBrushIndirect(const LOGBRUSH* lb)
{
  Brush* b = new Brush;
  b->style = lb->lbStyle == BS_NULL ? BS_NULL : BS_SOLID;
  b->color = lb->lbColor;
  return b;
}

HBRUSH WINAPI GetSysColorBrush(int i)
{
  return CreateSolidBrush(GetSysColor(i));
}

HFONT WINAPI CreateFontIndirectA(const LOGFONTA* lf)
{
  Font* f = new Font;
  f->lf = *lf;
  return f;
}

HFONT WINAPI CreateFontA(int h, int w, int esc, int ori, int weight, DWORD italic, DWORD underline,
                         DWORD strike, DWORD charset, DWORD outp, DWORD clip, DWORD quality,
                         DWORD pitch, LPCSTR face)
{
  LOGFONTA lf;
  memset(&lf, 0, sizeof(lf));
  lf.lfHeight = h;
  lf.lfWidth = w;
  lf.lfEscapement = esc;
  lf.lfOrientation = ori;
  lf.lfWeight = weight;
  lf.lfItalic = (BYTE)italic;
  lf.lfUnderline = (BYTE)underline;
  lf.lfStrikeOut = (BYTE)strike;
  lf.lfCharSet = (BYTE)charset;
  lf.lfOutPrecision = (BYTE)outp;
  lf.lfClipPrecision = (BYTE)clip;
  lf.lfQuality = (BYTE)quality;
  lf.lfPitchAndFamily = (BYTE)pitch;
  if (face)
    strncpy(lf.lfFaceName, face, LF_FACESIZE - 1);
  return CreateFontIndirectA(&lf);
}

COLORREF WINAPI SetDCBrushColor(HDC, COLORREF c)
{
  return c;
}

COLORREF WINAPI SetDCPenColor(HDC, COLORREF c)
{
  return c;
}

static Region* newRegion()
{
  return new Region;
}

HRGN WINAPI CreateRectRgn(int l, int t, int r, int b)
{
  Region* g = newRegion();
  if (l < r && t < b)
    g->rects.push_back({l, t, r, b});
  return g;
}

HRGN WINAPI CreateRectRgnIndirect(const RECT* r)
{
  return CreateRectRgn(r->left, r->top, r->right, r->bottom);
}

HRGN WINAPI CreateRoundRectRgn(int l, int t, int r, int b, int, int)
{
  return CreateRectRgn(l, t, r, b);
}

HRGN WINAPI CreateEllipticRgn(int l, int t, int r, int b)
{
  return CreateRectRgn(l, t, r, b);
}

static void subtractRect(const RECT& a, const RECT& b, std::vector<RECT>& out)
{
  RECT i = {std::max(a.left, b.left), std::max(a.top, b.top), std::min(a.right, b.right), std::min(a.bottom, b.bottom)};
  if (i.left >= i.right || i.top >= i.bottom) {
    out.push_back(a);
    return;
  }
  if (a.top < i.top)
    out.push_back({a.left, a.top, a.right, i.top});
  if (i.bottom < a.bottom)
    out.push_back({a.left, i.bottom, a.right, a.bottom});
  if (a.left < i.left)
    out.push_back({a.left, i.top, i.left, i.bottom});
  if (i.right < a.right)
    out.push_back({i.right, i.top, a.right, i.bottom});
}

static std::vector<RECT> rectsAnd(const std::vector<RECT>& a, const std::vector<RECT>& b)
{
  std::vector<RECT> out;
  for (const RECT& x : a)
    for (const RECT& y : b) {
      RECT i = {std::max(x.left, y.left), std::max(x.top, y.top), std::min(x.right, y.right), std::min(x.bottom, y.bottom)};
      if (i.left < i.right && i.top < i.bottom)
        out.push_back(i);
    }
  return out;
}

static std::vector<RECT> rectsDiff(const std::vector<RECT>& a, const std::vector<RECT>& b)
{
  std::vector<RECT> cur = a;
  for (const RECT& y : b) {
    std::vector<RECT> next;
    for (const RECT& x : cur)
      subtractRect(x, y, next);
    cur.swap(next);
  }
  return cur;
}

static int regionType(const std::vector<RECT>& r)
{
  return r.empty() ? NULLREGION : (r.size() == 1 ? SIMPLEREGION : COMPLEXREGION);
}

int WINAPI CombineRgn(HRGN dst, HRGN a, HRGN b, int mode)
{
  Region* d = (Region*)dst;
  Region* ra = (Region*)a;
  Region* rb = (Region*)b;
  if (!d || !ra)
    return ERROR;
  std::vector<RECT> res;
  switch (mode) {
    case RGN_AND: res = rectsAnd(ra->rects, rb->rects); break;
    case RGN_OR: res = rectsDiff(ra->rects, rb->rects); res.insert(res.end(), rb->rects.begin(), rb->rects.end()); break;
    case RGN_DIFF: res = rectsDiff(ra->rects, rb->rects); break;
    case RGN_XOR: {
      res = rectsDiff(ra->rects, rb->rects);
      std::vector<RECT> r2 = rectsDiff(rb->rects, ra->rects);
      res.insert(res.end(), r2.begin(), r2.end());
      break;
    }
    default: res = ra->rects; break;
  }
  d->rects = res;
  return regionType(res);
}

int WINAPI OffsetRgn(HRGN h, int dx, int dy)
{
  Region* g = (Region*)h;
  for (RECT& r : g->rects) {
    r.left += dx;
    r.right += dx;
    r.top += dy;
    r.bottom += dy;
  }
  return regionType(g->rects);
}

int WINAPI GetRgnBox(HRGN h, LPRECT out)
{
  Region* g = (Region*)h;
  if (g->rects.empty()) {
    *out = {0, 0, 0, 0};
    return NULLREGION;
  }
  RECT b = g->rects[0];
  for (const RECT& r : g->rects) {
    b.left = std::min(b.left, r.left);
    b.top = std::min(b.top, r.top);
    b.right = std::max(b.right, r.right);
    b.bottom = std::max(b.bottom, r.bottom);
  }
  *out = b;
  return regionType(g->rects);
}

BOOL WINAPI PtInRegion(HRGN h, int x, int y)
{
  for (const RECT& r : ((Region*)h)->rects)
    if (x >= r.left && x < r.right && y >= r.top && y < r.bottom)
      return TRUE;
  return FALSE;
}

int WINAPI SelectClipRgn(HDC h, HRGN rgn)
{
  DC* dc = D(h);
  if (!rgn) {
    dc->s.has_clip = false;
    dc->s.clip.clear();
    return SIMPLEREGION;
  }
  dc->s.has_clip = true;
  dc->s.clip = ((Region*)rgn)->rects;
  return regionType(dc->s.clip);
}

int WINAPI ExtSelectClipRgn(HDC h, HRGN rgn, int mode)
{
  DC* dc = D(h);
  if (mode == RGN_COPY || !dc->s.has_clip)
    return SelectClipRgn(h, rgn);
  if (!rgn)
    return ERROR;
  Region tmp;
  tmp.rects = dc->s.clip;
  Region out;
  CombineRgn((HRGN)&out, (HRGN)&tmp, rgn, mode);
  dc->s.clip = out.rects;
  return regionType(dc->s.clip);
}

static RECT logicalRectToDev(DC* dc, int l, int t, int r, int b)
{
  double x0, y0, x1, y1;
  devBox(dc, l, t, r, b, x0, y0, x1, y1);
  return {rnd(x0), rnd(y0), rnd(x1), rnd(y1)};
}

int WINAPI IntersectClipRect(HDC h, int l, int t, int r, int b)
{
  DC* dc = D(h);
  RECT d = logicalRectToDev(dc, l, t, r, b);
  if (!dc->s.has_clip) {
    dc->s.has_clip = true;
    dc->s.clip.clear();
    dc->s.clip.push_back(d);
  }
  else {
    dc->s.clip = rectsAnd(dc->s.clip, {d});
  }
  return regionType(dc->s.clip);
}

int WINAPI ExcludeClipRect(HDC h, int l, int t, int r, int b)
{
  DC* dc = D(h);
  RECT d = logicalRectToDev(dc, l, t, r, b);
  if (!dc->s.has_clip) {
    Bitmap* bm = dc->surface();
    RECT all = dc->window_dc ? RECT{0, 0, dc->wnd_area.right - dc->wnd_area.left, dc->wnd_area.bottom - dc->wnd_area.top}
                             : RECT{0, 0, bm ? bm->w : 0, bm ? bm->h : 0};
    dc->s.clip.assign(1, all);
    dc->s.has_clip = true;
  }
  dc->s.clip = rectsDiff(dc->s.clip, {d});
  return regionType(dc->s.clip);
}

int WINAPI GetClipBox(HDC h, LPRECT out)
{
  DC* dc = D(h);
  RECT dev;
  if (dc->s.has_clip) {
    Region tmp;
    tmp.rects = dc->s.clip;
    GetRgnBox((HRGN)&tmp, &dev);
  }
  else if (dc->window_dc) {
    dev = {0, 0, dc->wnd_area.right - dc->wnd_area.left, dc->wnd_area.bottom - dc->wnd_area.top};
  }
  else {
    Bitmap* b = dc->surface();
    dev = {0, 0, b ? b->w : 0, b ? b->h : 0};
  }
  if (dc->painting)
    dev = {std::max(dev.left, dc->paint_rect.left), std::max(dev.top, dc->paint_rect.top),
           std::min(dev.right, dc->paint_rect.right), std::min(dev.bottom, dc->paint_rect.bottom)};
  POINT p[2] = {{dev.left, dev.top}, {dev.right, dev.bottom}};
  DPtoLP(h, p, 2);
  *out = {std::min(p[0].x, p[1].x), std::min(p[0].y, p[1].y), std::max(p[0].x, p[1].x), std::max(p[0].y, p[1].y)};
  return dc->s.has_clip ? regionType(dc->s.clip) : SIMPLEREGION;
}

int WINAPI GetClipRgn(HDC h, HRGN rgn)
{
  DC* dc = D(h);
  if (!dc->s.has_clip)
    return 0;
  ((Region*)rgn)->rects = dc->s.clip;
  return 1;
}

BOOL WINAPI FillRgn(HDC h, HRGN rgn, HBRUSH br)
{
  for (const RECT& r : ((Region*)rgn)->rects)
    FillRect(h, &r, br);
  return TRUE;
}

int WINAPI SetGraphicsMode(HDC h, int mode)
{
  DC* dc = D(h);
  int prev = dc->s.gmode;
  dc->s.gmode = mode;
  return prev;
}

int WINAPI GetGraphicsMode(HDC h)
{
  return D(h)->s.gmode;
}

BOOL WINAPI SetWorldTransform(HDC h, const XFORM* xf)
{
  DC* dc = D(h);
  if (dc->s.gmode != GM_ADVANCED || !xf)
    return FALSE;
  dc->s.xf = *xf;
  return TRUE;
}

BOOL WINAPI GetWorldTransform(HDC h, LPXFORM xf)
{
  *xf = D(h)->s.xf;
  return TRUE;
}

static XFORM mul(const XFORM& a, const XFORM& b)
{
  XFORM r;
  r.eM11 = a.eM11 * b.eM11 + a.eM12 * b.eM21;
  r.eM12 = a.eM11 * b.eM12 + a.eM12 * b.eM22;
  r.eM21 = a.eM21 * b.eM11 + a.eM22 * b.eM21;
  r.eM22 = a.eM21 * b.eM12 + a.eM22 * b.eM22;
  r.eDx = a.eDx * b.eM11 + a.eDy * b.eM21 + b.eDx;
  r.eDy = a.eDx * b.eM12 + a.eDy * b.eM22 + b.eDy;
  return r;
}

BOOL WINAPI ModifyWorldTransform(HDC h, const XFORM* xf, DWORD mode)
{
  DC* dc = D(h);
  if (dc->s.gmode != GM_ADVANCED)
    return FALSE;
  if (mode == MWT_IDENTITY)
    dc->s.xf = {1, 0, 0, 1, 0, 0};
  else if (mode == MWT_LEFTMULTIPLY && xf)
    dc->s.xf = mul(*xf, dc->s.xf);
  else if (mode == MWT_RIGHTMULTIPLY && xf)
    dc->s.xf = mul(dc->s.xf, *xf);
  return TRUE;
}

BOOL WINAPI LPtoDP(HDC h, LPPOINT p, int n)
{
  DC* dc = D(h);
  for (int i = 0; i < n; i++) {
    double x, y;
    devPoint(dc, p[i].x, p[i].y, x, y);
    p[i].x = rnd(x);
    p[i].y = rnd(y);
  }
  return TRUE;
}

BOOL WINAPI DPtoLP(HDC h, LPPOINT p, int n)
{
  DC* dc = D(h);
  for (int i = 0; i < n; i++) {
    double x = p[i].x - (dc->s.vp_org.x - dc->s.win_org.x), y = p[i].y - (dc->s.vp_org.y - dc->s.win_org.y);
    if (dc->s.gmode == GM_ADVANCED) {
      const XFORM& f = dc->s.xf;
      double det = (double)f.eM11 * f.eM22 - (double)f.eM12 * f.eM21;
      if (det != 0) {
        double tx = x - f.eDx, ty = y - f.eDy;
        x = (tx * f.eM22 - ty * f.eM21) / det;
        y = (ty * f.eM11 - tx * f.eM12) / det;
      }
    }
    p[i].x = rnd(x);
    p[i].y = rnd(y);
  }
  return TRUE;
}

BOOL WINAPI SetViewportOrgEx(HDC h, int x, int y, LPPOINT old)
{
  DC* dc = D(h);
  if (old)
    *old = dc->s.vp_org;
  dc->s.vp_org = {x, y};
  return TRUE;
}

BOOL WINAPI GetViewportOrgEx(HDC h, LPPOINT p)
{
  *p = D(h)->s.vp_org;
  return TRUE;
}

BOOL WINAPI SetWindowOrgEx(HDC h, int x, int y, LPPOINT old)
{
  DC* dc = D(h);
  if (old)
    *old = dc->s.win_org;
  dc->s.win_org = {x, y};
  return TRUE;
}

BOOL WINAPI OffsetViewportOrgEx(HDC h, int x, int y, LPPOINT old)
{
  DC* dc = D(h);
  if (old)
    *old = dc->s.vp_org;
  dc->s.vp_org.x += x;
  dc->s.vp_org.y += y;
  return TRUE;
}

BOOL WINAPI SetBrushOrgEx(HDC h, int x, int y, LPPOINT old)
{
  DC* dc = D(h);
  if (old)
    *old = dc->s.brush_org;
  dc->s.brush_org = {x, y};
  return TRUE;
}

COLORREF WINAPI SetTextColor(HDC h, COLORREF c)
{
  COLORREF p = D(h)->s.text;
  D(h)->s.text = c;
  return p;
}

COLORREF WINAPI GetTextColor(HDC h)
{
  return D(h)->s.text;
}

COLORREF WINAPI SetBkColor(HDC h, COLORREF c)
{
  COLORREF p = D(h)->s.bk;
  D(h)->s.bk = c;
  return p;
}

COLORREF WINAPI GetBkColor(HDC h)
{
  return D(h)->s.bk;
}

int WINAPI SetBkMode(HDC h, int m)
{
  int p = D(h)->s.bk_mode;
  D(h)->s.bk_mode = m;
  return p;
}

int WINAPI GetBkMode(HDC h)
{
  return D(h)->s.bk_mode;
}

int WINAPI SetROP2(HDC h, int m)
{
  int p = D(h)->s.rop2;
  D(h)->s.rop2 = m;
  return p;
}

int WINAPI GetROP2(HDC h)
{
  return D(h)->s.rop2;
}

int WINAPI SetStretchBltMode(HDC h, int m)
{
  int p = D(h)->s.stretch_mode;
  D(h)->s.stretch_mode = m;
  return p;
}

int WINAPI SetPolyFillMode(HDC h, int m)
{
  int p = D(h)->s.poly_fill;
  D(h)->s.poly_fill = m;
  return p;
}

UINT WINAPI SetTextAlign(HDC h, UINT a)
{
  UINT p = D(h)->s.text_align;
  D(h)->s.text_align = a;
  return p;
}

BOOL WINAPI MoveToEx(HDC h, int x, int y, LPPOINT old)
{
  DC* dc = D(h);
  if (old)
    *old = dc->s.pos;
  dc->s.pos = {x, y};
  return TRUE;
}

BOOL WINAPI LineTo(HDC h, int x, int y)
{
  DC* dc = D(h);
  beginPrimitive(dc);
  POINT p[2] = {dc->s.pos, {x, y}};
  std::vector<double> xs, ys;
  toDev(dc, p, 2, xs, ys);
  strokeDev(dc, xs, ys, false);
  dc->s.pos = {x, y};
  return TRUE;
}

BOOL WINAPI Polyline(HDC h, const POINT* p, int n)
{
  DC* dc = D(h);
  if (n < 2)
    return FALSE;
  beginPrimitive(dc);
  std::vector<double> xs, ys;
  toDev(dc, p, n, xs, ys);
  strokeDev(dc, xs, ys, false);
  return TRUE;
}

BOOL WINAPI PolylineTo(HDC h, const POINT* p, DWORD n)
{
  DC* dc = D(h);
  std::vector<POINT> pts;
  pts.push_back(dc->s.pos);
  pts.insert(pts.end(), p, p + n);
  Polyline(h, pts.data(), (int)pts.size());
  if (n)
    dc->s.pos = p[n - 1];
  return TRUE;
}

static void fillAndStroke(DC* dc, const std::vector<double>& xs, const std::vector<double>& ys)
{
  beginPrimitive(dc);
  if (brushOn(dc))
    fillPolygonDev(dc, xs, ys, {(int)xs.size()}, toPixel(dc->s.brush->color), dc->s.poly_fill);
  strokeDev(dc, xs, ys, true);
}

BOOL WINAPI Polygon(HDC h, const POINT* p, int n)
{
  DC* dc = D(h);
  if (n < 2)
    return FALSE;
  std::vector<double> xs, ys;
  toDev(dc, p, n, xs, ys);
  fillAndStroke(dc, xs, ys);
  return TRUE;
}

BOOL WINAPI PolyPolygon(HDC h, const POINT* p, const INT* counts, int n)
{
  DC* dc = D(h);
  beginPrimitive(dc);
  std::vector<double> xs, ys;
  std::vector<int> c;
  int total = 0;
  for (int i = 0; i < n; i++) {
    c.push_back(counts[i]);
    total += counts[i];
  }
  toDev(dc, p, total, xs, ys);
  if (brushOn(dc))
    fillPolygonDev(dc, xs, ys, c, toPixel(dc->s.brush->color), dc->s.poly_fill);
  int base = 0;
  for (int i = 0; i < n; i++) {
    std::vector<double> sx(xs.begin() + base, xs.begin() + base + counts[i]);
    std::vector<double> sy(ys.begin() + base, ys.begin() + base + counts[i]);
    strokeDev(dc, sx, sy, true);
    base += counts[i];
  }
  return TRUE;
}

BOOL WINAPI Rectangle(HDC h, int l, int t, int r, int b)
{
  DC* dc = D(h);
  beginPrimitive(dc);
  double x0, y0, x1, y1;
  devBox(dc, l, t, r, b, x0, y0, x1, y1);
  int ix0 = rnd(x0), iy0 = rnd(y0), ix1 = rnd(x1), iy1 = rnd(y1);
  if (dc->s.gmode == GM_ADVANCED) {
    ix1++;
    iy1++;
  }
  int pw = penPixels(dc);
  int ox = dc->ox, oy = dc->oy;
  if (brushOn(dc)) {
    uint32_t px = toPixel(dc->s.brush->color);
    int inset = pw > 0 ? (pw == 1 ? 1 : pw / 2) : 0;
    for (int y = iy0 + inset; y < iy1 - inset; y++)
      spanRaw(dc, t_clips, y + oy, ix0 + inset + ox, ix1 - inset + ox, px);
  }
  if (pw > 0) {
    uint32_t px = toPixel(dc->s.pen->color);
    if (pw == 1 && (dc->s.pen->style & PS_STYLE_MASK) == PS_SOLID) {
      spanRaw(dc, t_clips, iy0 + oy, ix0 + ox, ix1 + ox, px);
      spanRaw(dc, t_clips, iy1 - 1 + oy, ix0 + ox, ix1 + ox, px);
      for (int y = iy0; y < iy1; y++) {
        spanRaw(dc, t_clips, y + oy, ix0 + ox, ix0 + 1 + ox, px);
        spanRaw(dc, t_clips, y + oy, ix1 - 1 + ox, ix1 + ox, px);
      }
    }
    else {
      std::vector<double> xs = {(double)ix0, (double)ix1 - 1, (double)ix1 - 1, (double)ix0};
      std::vector<double> ys = {(double)iy0, (double)iy0, (double)iy1 - 1, (double)iy1 - 1};
      strokeDev(dc, xs, ys, true);
    }
  }
  touched(dc, ix0 + ox, iy0 + oy, ix1 + ox, iy1 + oy);
  return TRUE;
}

BOOL WINAPI RoundRect(HDC h, int l, int t, int r, int b, int ew, int eh)
{
  DC* dc = D(h);
  double x0, y0, x1, y1;
  devBox(dc, l, t, r, b, x0, y0, x1, y1);
  x1 -= 1;
  y1 -= 1;
  double s = devScale(dc);
  double rx = std::min(ew * s / 2.0, (x1 - x0) / 2.0), ry = std::min(eh * s / 2.0, (y1 - y0) / 2.0);
  if (rx <= 0.5 || ry <= 0.5)
    return Rectangle(h, l, t, r, b);
  std::vector<double> xs, ys;
  auto corner = [&](double cx, double cy, double a0) {
    int n = std::max(4, (int)(std::max(rx, ry)));
    for (int i = 0; i <= n; i++) {
      double a = a0 + 1.5707963267948966 * i / n;
      xs.push_back(cx + std::cos(a) * rx);
      ys.push_back(cy - std::sin(a) * ry);
    }
  };
  corner(x1 - rx, y0 + ry, 0.0);
  corner(x0 + rx, y0 + ry, 1.5707963267948966);
  corner(x0 + rx, y1 - ry, 3.141592653589793);
  corner(x1 - rx, y1 - ry, 4.71238898038469);
  std::vector<double> fx = xs, fy = ys;
  for (size_t i = 0; i < fx.size(); i++) {
    fx[i] += fx[i] >= (x0 + x1) / 2 ? 0.5 : -0.0;
    fy[i] += fy[i] >= (y0 + y1) / 2 ? 0.5 : -0.0;
  }
  beginPrimitive(dc);
  if (brushOn(dc))
    fillPolygonDev(dc, fx, fy, {(int)fx.size()}, toPixel(dc->s.brush->color), WINDING);
  strokeDev(dc, xs, ys, true);
  return TRUE;
}

BOOL WINAPI Ellipse(HDC h, int l, int t, int r, int b)
{
  DC* dc = D(h);
  double x0, y0, x1, y1;
  devBox(dc, l, t, r, b, x0, y0, x1, y1);
  std::vector<double> xs, ys;
  ellipsePoints(x0, y0, x1, y1, 0.0, 6.283185307179586, xs, ys);
  xs.pop_back();
  ys.pop_back();
  beginPrimitive(dc);
  if (brushOn(dc))
    fillPolygonDev(dc, xs, ys, {(int)xs.size()}, toPixel(dc->s.brush->color), WINDING);
  std::vector<double> sx(xs), sy(ys);
  for (size_t i = 0; i < sx.size(); i++) {
    sx[i] = std::min(sx[i], x1 - 1);
    sy[i] = std::min(sy[i], y1 - 1);
  }
  strokeDev(dc, sx, sy, true);
  return TRUE;
}

static double angleOf(double cx, double cy, double x, double y)
{
  return std::atan2(-(y - cy), x - cx);
}

static void arcLike(HDC h, int l, int t, int r, int b, int xs_, int ys_, int xe, int ye, int kind)
{
  DC* dc = D(h);
  double x0, y0, x1, y1;
  devBox(dc, l, t, r, b, x0, y0, x1, y1);
  double sx, sy, ex, ey;
  devPoint(dc, xs_, ys_, sx, sy);
  devPoint(dc, xe, ye, ex, ey);
  double cx = (x0 + x1) / 2.0, cy = (y0 + y1) / 2.0;
  double a0 = angleOf(cx, cy, sx, sy), a1 = angleOf(cx, cy, ex, ey);
  if (a1 <= a0)
    a1 += 6.283185307179586;
  std::vector<double> xs, ys;
  ellipsePoints(x0, y0, x1, y1, a0, a1, xs, ys);
  beginPrimitive(dc);
  if (kind == 0) {
    strokeDev(dc, xs, ys, false);
    return;
  }
  if (kind == 1) {
    xs.push_back(cx);
    ys.push_back(cy);
  }
  if (brushOn(dc))
    fillPolygonDev(dc, xs, ys, {(int)xs.size()}, toPixel(dc->s.brush->color), WINDING);
  strokeDev(dc, xs, ys, true);
}

BOOL WINAPI Arc(HDC h, int l, int t, int r, int b, int x1, int y1, int x2, int y2)
{
  arcLike(h, l, t, r, b, x1, y1, x2, y2, 0);
  return TRUE;
}

BOOL WINAPI Pie(HDC h, int l, int t, int r, int b, int x1, int y1, int x2, int y2)
{
  arcLike(h, l, t, r, b, x1, y1, x2, y2, 1);
  return TRUE;
}

BOOL WINAPI Chord(HDC h, int l, int t, int r, int b, int x1, int y1, int x2, int y2)
{
  arcLike(h, l, t, r, b, x1, y1, x2, y2, 2);
  return TRUE;
}

int WINAPI FillRect(HDC h, const RECT* r, HBRUSH br)
{
  DC* dc = D(h);
  Brush* b = (Brush*)br;
  if ((uintptr_t)br <= 64 && (uintptr_t)br > 0)
    b = (Brush*)GetSysColorBrush((int)(uintptr_t)br - 1);
  if (!b || b->style == BS_NULL || !r)
    return 0;
  beginPrimitive(dc);
  RECT d = logicalRectToDev(dc, r->left, r->top, r->right, r->bottom);
  uint32_t px = toPixel(b->color);
  int saved = dc->s.rop2;
  dc->s.rop2 = R2_COPYPEN;
  for (int y = d.top; y < d.bottom; y++)
    spanRaw(dc, t_clips, y + dc->oy, d.left + dc->ox, d.right + dc->ox, px);
  dc->s.rop2 = saved;
  touched(dc, d.left + dc->ox, d.top + dc->oy, d.right + dc->ox, d.bottom + dc->oy);
  return 1;
}

int WINAPI FrameRect(HDC h, const RECT* r, HBRUSH br)
{
  RECT a = {r->left, r->top, r->right, r->top + 1};
  FillRect(h, &a, br);
  a = {r->left, r->bottom - 1, r->right, r->bottom};
  FillRect(h, &a, br);
  a = {r->left, r->top, r->left + 1, r->bottom};
  FillRect(h, &a, br);
  a = {r->right - 1, r->top, r->right, r->bottom};
  FillRect(h, &a, br);
  return 1;
}

BOOL WINAPI InvertRect(HDC h, const RECT* r)
{
  DC* dc = D(h);
  beginPrimitive(dc);
  RECT d = logicalRectToDev(dc, r->left, r->top, r->right, r->bottom);
  int saved = dc->s.rop2;
  dc->s.rop2 = R2_NOT;
  for (int y = d.top; y < d.bottom; y++)
    spanRaw(dc, t_clips, y + dc->oy, d.left + dc->ox, d.right + dc->ox, 0);
  dc->s.rop2 = saved;
  touched(dc, d.left + dc->ox, d.top + dc->oy, d.right + dc->ox, d.bottom + dc->oy);
  return TRUE;
}

static bool insideClips(const std::vector<RECT>& clips, int x, int y)
{
  for (const RECT& c : clips)
    if (x >= c.left && x < c.right && y >= c.top && y < c.bottom)
      return true;
  return false;
}

COLORREF WINAPI SetPixel(HDC h, int x, int y, COLORREF c)
{
  DC* dc = D(h);
  beginPrimitive(dc);
  double dx, dy;
  devPoint(dc, x, y, dx, dy);
  int sx = (int)std::floor(dx) + dc->ox, sy = (int)std::floor(dy) + dc->oy;
  if (!insideClips(t_clips, sx, sy))
    return CLR_INVALID;
  dc->surface()->set(sx, sy, toPixel(c));
  touched(dc, sx, sy, sx + 1, sy + 1);
  return c;
}

BOOL WINAPI SetPixelV(HDC h, int x, int y, COLORREF c)
{
  return SetPixel(h, x, y, c) != CLR_INVALID;
}

COLORREF WINAPI GetPixel(HDC h, int x, int y)
{
  DC* dc = D(h);
  beginPrimitive(dc);
  double dx, dy;
  devPoint(dc, x, y, dx, dy);
  int sx = (int)std::floor(dx) + dc->ox, sy = (int)std::floor(dy) + dc->oy;
  if (!insideClips(t_clips, sx, sy))
    return CLR_INVALID;
  return fromPixel(dc->surface()->get(sx, sy));
}

BOOL WINAPI ExtFloodFill(HDC h, int x, int y, COLORREF c, UINT type)
{
  DC* dc = D(h);
  beginPrimitive(dc);
  Bitmap* b = dc->surface();
  if (!b || b->bpp != 32 || !brushOn(dc))
    return FALSE;
  double dx, dy;
  devPoint(dc, x, y, dx, dy);
  int sx = (int)std::floor(dx) + dc->ox, sy = (int)std::floor(dy) + dc->oy;
  if (!insideClips(t_clips, sx, sy))
    return FALSE;
  uint32_t ref = toPixel(c), fill = toPixel(dc->s.brush->color);
  auto match = [&](int px, int py) {
    uint32_t v = b->get(px, py) & 0x00ffffffu;
    return type == FLOODFILLSURFACE ? v == ref : v != ref;
  };
  if (!match(sx, sy) || (b->get(sx, sy) & 0x00ffffffu) == fill)
    return FALSE;
  std::vector<uint8_t> seen((size_t)b->w * b->h, 0);
  std::deque<std::pair<int, int>> q;
  q.push_back({sx, sy});
  seen[(size_t)sy * b->w + sx] = 1;
  while (!q.empty()) {
    auto [px, py] = q.front();
    q.pop_front();
    b->set(px, py, fill);
    const int nx[4] = {px + 1, px - 1, px, px}, ny[4] = {py, py, py + 1, py - 1};
    for (int k = 0; k < 4; k++) {
      int qx = nx[k], qy = ny[k];
      if (qx < 0 || qy < 0 || qx >= b->w || qy >= b->h || seen[(size_t)qy * b->w + qx])
        continue;
      seen[(size_t)qy * b->w + qx] = 1;
      if (insideClips(t_clips, qx, qy) && match(qx, qy))
        q.push_back({qx, qy});
    }
  }
  touched(dc, 0, 0, b->w, b->h);
  return TRUE;
}

BOOL WINAPI FloodFill(HDC h, int x, int y, COLORREF c)
{
  return ExtFloodFill(h, x, y, c, FLOODFILLBORDER);
}

BOOL WINAPI GdiFlush()
{
  return TRUE;
}

namespace softwin {

static uint32_t ropApply(DWORD rop, uint32_t d, uint32_t s, uint32_t p)
{
  switch (rop) {
    case SRCCOPY: return s;
    case SRCPAINT: return d | s;
    case SRCAND: return d & s;
    case SRCINVERT: return d ^ s;
    case SRCERASE: return s & ~d;
    case NOTSRCCOPY: return ~s;
    case NOTSRCERASE: return ~(s | d);
    case MERGEPAINT: return ~s | d;
    case PATCOPY: return p;
    case PATINVERT: return d ^ p;
    case DSTINVERT: return ~d;
    case BLACKNESS: return 0;
    case WHITENESS: return 0x00ffffffu;
    case 0x00B8074A: return (s & p) | (~s & d);
    case 0x00E20746: return (s & d) | (~s & p);
    default: return s;
  }
}

static bool ropUsesSrc(DWORD rop)
{
  return !(rop == PATCOPY || rop == PATINVERT || rop == DSTINVERT || rop == BLACKNESS || rop == WHITENESS);
}

struct SrcRect {
  Bitmap* b;
  DC* dc;
  double x0, y0, x1, y1;
};

static uint32_t srcPixelFor(SrcRect& s, DC* dst, int x, int y)
{
  uint32_t v = s.b->get(x, y);
  if (s.b->bpp == 1 && dst->surface() && dst->surface()->bpp == 32)
    return v ? toPixel(dst->s.bk) : toPixel(dst->s.text);
  if (s.b->bpp == 32 && dst->surface() && dst->surface()->bpp == 1)
    return ((v & 0x00ffffffu) == toPixel(s.dc->s.bk)) ? 0x00ffffffu : 0;
  return v;
}

static bool sampleBox(SrcRect& s, DC* dst, double fx0, double fy0, double fx1, double fy1, bool average, uint32_t& out)
{
  int bx0 = (int)std::floor(fx0), by0 = (int)std::floor(fy0);
  int bx1 = std::max(bx0 + 1, (int)std::ceil(fx1)), by1 = std::max(by0 + 1, (int)std::ceil(fy1));
  if (!average) {
    int x = (int)std::floor((fx0 + fx1) * 0.5), y = (int)std::floor((fy0 + fy1) * 0.5);
    if (x < 0 || y < 0 || x >= s.b->w || y >= s.b->h)
      return false;
    out = srcPixelFor(s, dst, x, y);
    return true;
  }
  double acc[4] = {0, 0, 0, 0}, wsum = 0;
  for (int y = by0; y < by1; y++) {
    if (y < 0 || y >= s.b->h)
      continue;
    double wy = std::min((double)y + 1, fy1) - std::max((double)y, fy0);
    if (wy <= 0)
      continue;
    for (int x = bx0; x < bx1; x++) {
      if (x < 0 || x >= s.b->w)
        continue;
      double wx = std::min((double)x + 1, fx1) - std::max((double)x, fx0);
      if (wx <= 0)
        continue;
      uint32_t v = srcPixelFor(s, dst, x, y);
      double w = wx * wy;
      acc[0] += w * (v & 0xff);
      acc[1] += w * ((v >> 8) & 0xff);
      acc[2] += w * ((v >> 16) & 0xff);
      acc[3] += w * ((v >> 24) & 0xff);
      wsum += w;
    }
  }
  if (wsum <= 0)
    return false;
  uint32_t c[4];
  for (int k = 0; k < 4; k++)
    c[k] = (uint32_t)std::min(255.0, std::floor(acc[k] / wsum + 0.5));
  out = c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);
  return true;
}

template <class F>
static BOOL blitGeneric(HDC hdst, int x, int y, int w, int h, HDC hsrc, int sx, int sy, int sw, int sh, bool average, F&& combine)
{
  DC* dst = D(hdst);
  DC* src = D(hsrc);
  if (!dst || !dst->surface() || w == 0 || h == 0)
    return FALSE;
  beginPrimitive(dst);
  std::vector<RECT> clips = t_clips;
  double dx0, dy0, dx1, dy1;
  devPoint(dst, x, y, dx0, dy0);
  devPoint(dst, x + w, y + h, dx1, dy1);
  SrcRect s = {nullptr, src, 0, 0, 0, 0};
  if (src && src->surface()) {
    s.b = src->surface();
    devPoint(src, sx, sy, s.x0, s.y0);
    devPoint(src, sx + sw, sy + sh, s.x1, s.y1);
    s.x0 += src->ox;
    s.x1 += src->ox;
    s.y0 += src->oy;
    s.y1 += src->oy;
  }
  int ix0 = rnd(std::min(dx0, dx1)), ix1 = rnd(std::max(dx0, dx1));
  int iy0 = rnd(std::min(dy0, dy1)), iy1 = rnd(std::max(dy0, dy1));
  bool flipx = dx1 < dx0, flipy = dy1 < dy0;
  if (s.b) {
    if (s.x1 < s.x0)
      flipx = !flipx;
    if (s.y1 < s.y0)
      flipy = !flipy;
  }
  double sxa = s.b ? std::min(s.x0, s.x1) : 0, sya = s.b ? std::min(s.y0, s.y1) : 0;
  double sxw = s.b ? std::fabs(s.x1 - s.x0) : 0, syh = s.b ? std::fabs(s.y1 - s.y0) : 0;
  int dw = ix1 - ix0, dh = iy1 - iy0;
  if (dw <= 0 || dh <= 0)
    return TRUE;
  Bitmap* db = dst->surface();
  bool same_size = s.b && rnd(sxw) == dw && rnd(syh) == dh;
  for (int yy = 0; yy < dh; yy++) {
    int ty = iy0 + yy + dst->oy;
    if (ty < 0 || ty >= db->h)
      continue;
    for (int xx = 0; xx < dw; xx++) {
      int tx = ix0 + xx + dst->ox;
      if (tx < 0 || tx >= db->w || !insideClips(clips, tx, ty))
        continue;
      uint32_t sv = 0;
      bool have = true;
      if (s.b) {
        int ux = flipx ? dw - 1 - xx : xx, uy = flipy ? dh - 1 - yy : yy;
        if (same_size) {
          int px = rnd(sxa) + ux, py = rnd(sya) + uy;
          if (px < 0 || py < 0 || px >= s.b->w || py >= s.b->h)
            have = false;
          else
            sv = srcPixelFor(s, dst, px, py);
        }
        else {
          double fx0 = sxa + sxw * ux / dw, fx1 = sxa + sxw * (ux + 1) / dw;
          double fy0 = sya + syh * uy / dh, fy1 = sya + syh * (uy + 1) / dh;
          have = sampleBox(s, dst, fx0, fy0, fx1, fy1, average, sv);
        }
      }
      if (!have)
        continue;
      db->set(tx, ty, combine(db->get(tx, ty), sv));
    }
  }
  touched(dst, ix0 + dst->ox, iy0 + dst->oy, ix1 + dst->ox, iy1 + dst->oy);
  return TRUE;
}

}

BOOL WINAPI StretchBlt(HDC hdst, int x, int y, int w, int h, HDC hsrc, int sx, int sy, int sw, int sh, DWORD rop)
{
  DC* dst = D(hdst);
  uint32_t pat = dst && dst->s.brush ? toPixel(dst->s.brush->color) : 0;
  if (!ropUsesSrc(rop))
    hsrc = NULL;
  bool average = dst && dst->s.stretch_mode == HALFTONE;
  bool mono_dst = dst && dst->surface() && dst->surface()->bpp == 1;
  return blitGeneric(hdst, x, y, w, h, hsrc, sx, sy, sw, sh, average, [&](uint32_t d, uint32_t s) {
    uint32_t r = ropApply(rop, d, s, pat);
    if (mono_dst)
      return (r & 0x00ffffffu) == 0x00ffffffu ? 0x00ffffffu : 0u;
    if (rop == SRCCOPY)
      return r;
    return r;
  });
}

BOOL WINAPI BitBlt(HDC hdst, int x, int y, int w, int h, HDC hsrc, int sx, int sy, DWORD rop)
{
  return StretchBlt(hdst, x, y, w, h, hsrc, sx, sy, w, h, rop);
}

BOOL WINAPI PatBlt(HDC hdst, int x, int y, int w, int h, DWORD rop)
{
  return StretchBlt(hdst, x, y, w, h, NULL, 0, 0, w, h, rop);
}

BOOL WINAPI AlphaBlend(HDC hdst, int x, int y, int w, int h, HDC hsrc, int sx, int sy, int sw, int sh, BLENDFUNCTION bf)
{
  uint32_t ca = bf.SourceConstantAlpha;
  bool src_alpha = (bf.AlphaFormat & AC_SRC_ALPHA) != 0;
  return blitGeneric(hdst, x, y, w, h, hsrc, sx, sy, sw, sh, true, [&](uint32_t d, uint32_t s) {
    uint32_t sc[4], dc_[4];
    for (int k = 0; k < 4; k++) {
      sc[k] = (s >> (8 * k)) & 0xff;
      dc_[k] = (d >> (8 * k)) & 0xff;
    }
    uint32_t out[4];
    if (src_alpha) {
      for (int k = 0; k < 4; k++)
        sc[k] = (sc[k] * ca + 127) / 255;
      uint32_t inv = 255 - sc[3];
      for (int k = 0; k < 4; k++)
        out[k] = std::min<uint32_t>(255, sc[k] + (dc_[k] * inv + 127) / 255);
    }
    else {
      for (int k = 0; k < 3; k++)
        out[k] = (sc[k] * ca + dc_[k] * (255 - ca) + 127) / 255;
      out[3] = std::min<uint32_t>(255, ca + (dc_[3] * (255 - ca) + 127) / 255);
    }
    return out[0] | (out[1] << 8) | (out[2] << 16) | (out[3] << 24);
  });
}

BOOL WINAPI TransparentBlt(HDC hdst, int x, int y, int w, int h, HDC hsrc, int sx, int sy, int sw, int sh, UINT key)
{
  uint32_t k = toPixel(key);
  return blitGeneric(hdst, x, y, w, h, hsrc, sx, sy, sw, sh, false, [&](uint32_t d, uint32_t s) {
    return (s & 0x00ffffffu) == k ? d : s;
  });
}

BOOL WINAPI MaskBlt(HDC hdst, int x, int y, int w, int h, HDC hsrc, int sx, int sy, HBITMAP, int, int, DWORD rop)
{
  return BitBlt(hdst, x, y, w, h, hsrc, sx, sy, rop & 0x00FFFFFF ? SRCCOPY : SRCCOPY);
}

int WINAPI GetDeviceCaps(HDC, int idx)
{
  switch (idx) {
    case LOGPIXELSX:
    case LOGPIXELSY: return 96;
    case BITSPIXEL: return 32;
    case HORZRES: return 2560;
    case VERTRES: return 1600;
  }
  return 0;
}
