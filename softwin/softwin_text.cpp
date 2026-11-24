#include "softwin_internal.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <unordered_map>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

namespace softwin {

static std::vector<std::string> g_font_dirs = {
#if defined(__APPLE__)
    "/System/Library/Fonts/Supplemental/",
    "/Library/Fonts/",
    "/System/Library/Fonts/",
#else
    "/mnt/c/Windows/Fonts/",
    "/usr/share/fonts/truetype/msttcorefonts/",
    "/usr/share/fonts/truetype/dejavu/",
#endif
};

void setFontDirs(const char* const* dirs, int n)
{
  g_font_dirs.clear();
  for (int i = 0; i < n; i++)
    g_font_dirs.push_back(dirs[i]);
}

struct FontFile {
  std::vector<unsigned char> data;
  stbtt_fontinfo info;
  int ascent = 0, descent = 0, line_gap = 0, units_per_em = 2048;
};

static std::mutex g_mutex;
static std::map<std::string, std::unique_ptr<FontFile>> g_files;

static FontFile* loadFile(const std::string& path)
{
  auto it = g_files.find(path);
  if (it != g_files.end())
    return it->second.get();
  FILE* f = ::fopen(path.c_str(), "rb");
  if (!f) {
    g_files[path] = nullptr;
    return nullptr;
  }
  std::unique_ptr<FontFile> ff(new FontFile);
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  ff->data.resize(n > 0 ? (size_t)n : 0);
  size_t got = n > 0 ? fread(ff->data.data(), 1, (size_t)n, f) : 0;
  ::fclose(f);
  if (got != (size_t)n || !stbtt_InitFont(&ff->info, ff->data.data(), stbtt_GetFontOffsetForIndex(ff->data.data(), 0))) {
    g_files[path] = nullptr;
    return nullptr;
  }
  stbtt_GetFontVMetrics(&ff->info, &ff->ascent, &ff->descent, &ff->line_gap);
  int x0, y0, x1, y1;
  stbtt_GetFontBoundingBox(&ff->info, &x0, &y0, &x1, &y1);
  ff->units_per_em = ff->info.data ? ttUSHORT(ff->info.data + ff->info.head + 18) : 2048;
  FontFile* raw = ff.get();
  g_files[path] = std::move(ff);
  return raw;
}

static std::string lower(std::string s)
{
  for (char& c : s)
    c = (char)tolower((unsigned char)c);
  return s;
}

static FontFile* findFont(const char* face, bool bold, bool italic)
{
  std::string f = lower(face && *face ? face : "arial");
  std::vector<std::string> names;
  if (f.find("courier") != std::string::npos) {
    names = bold ? std::vector<std::string>{"Courier New Bold.ttf", "courbd.ttf"} : std::vector<std::string>{"Courier New.ttf", "cour.ttf"};
  }
  else if (f.find("tahoma") != std::string::npos) {
    names = bold ? std::vector<std::string>{"Tahoma Bold.ttf", "tahomabd.ttf"} : std::vector<std::string>{"Tahoma.ttf", "tahoma.ttf"};
  }
  else if (f.find("verdana") != std::string::npos) {
    names = bold ? std::vector<std::string>{"Verdana Bold.ttf", "verdanab.ttf"} : std::vector<std::string>{"Verdana.ttf", "verdana.ttf"};
  }
  if (bold && italic)
    names.insert(names.end(), {"Arial Bold Italic.ttf", "arialbi.ttf"});
  else if (bold)
    names.insert(names.end(), {"Arial Bold.ttf", "arialbd.ttf"});
  else if (italic)
    names.insert(names.end(), {"Arial Italic.ttf", "ariali.ttf"});
  names.insert(names.end(), {"Arial.ttf", "arial.ttf", "DejaVuSans.ttf"});
  for (const std::string& n : names)
    for (const std::string& d : g_font_dirs)
      if (FontFile* ff = loadFile(d + n))
        return ff;
  return nullptr;
}

struct Sized {
  FontFile* file = nullptr;
  float scale = 1.0f;
  int ascent = 0, descent = 0, em = 0;
  bool underline = false;
};

static Sized sizedFont(DC* dc)
{
  Sized s;
  Font* font = dc->s.font;
  const LOGFONTA& lf = font->lf;
  s.file = findFont(lf.lfFaceName, lf.lfWeight >= FW_SEMIBOLD, lf.lfItalic != 0);
  if (!s.file)
    return s;
  double k = devScale(dc);
  if (lf.lfHeight < 0) {
    s.scale = stbtt_ScaleForMappingEmToPixels(&s.file->info, (float)(-lf.lfHeight * k));
  }
  else if (lf.lfHeight > 0) {
    s.scale = stbtt_ScaleForPixelHeight(&s.file->info, (float)(lf.lfHeight * k));
  }
  else {
    s.scale = stbtt_ScaleForMappingEmToPixels(&s.file->info, (float)(12 * k));
  }
  s.ascent = (int)std::ceil(s.file->ascent * s.scale);
  s.descent = (int)std::ceil(-s.file->descent * s.scale);
  s.em = (int)std::lround(s.file->units_per_em * s.scale);
  s.underline = lf.lfUnderline != 0;
  return s;
}

int deviceFontPixels(DC* dc, int* ascent, int* descent, int* em)
{
  std::lock_guard<std::mutex> lock(g_mutex);
  Sized s = sizedFont(dc);
  if (ascent)
    *ascent = s.ascent;
  if (descent)
    *descent = s.descent;
  if (em)
    *em = s.em;
  return s.ascent + s.descent;
}

static int advancePx(const Sized& s, int cp)
{
  int adv, lsb;
  stbtt_GetCodepointHMetrics(&s.file->info, cp, &adv, &lsb);
  return (int)std::lround(adv * s.scale);
}

int textExtent(DC* dc, const char* str, int n, int* ascent, int* descent)
{
  std::lock_guard<std::mutex> lock(g_mutex);
  Sized s = sizedFont(dc);
  if (ascent)
    *ascent = s.ascent;
  if (descent)
    *descent = s.descent;
  if (!s.file)
    return n * 7;
  int w = 0;
  for (int i = 0; i < n; i++)
    w += advancePx(s, (unsigned char)str[i]);
  return w;
}

void drawTextRun(DC* dc, double dev_x, double dev_baseline_y, const char* str, int n, COLORREF color)
{
  std::lock_guard<std::mutex> lock(g_mutex);
  Sized s = sizedFont(dc);
  if (!s.file || n <= 0)
    return;
  beginPrimitive(dc);
  uint32_t pixel = toPixel(color);
  int pen_x = (int)std::lround(dev_x);
  int base_y = (int)std::lround(dev_baseline_y);
  int x_start = pen_x;
  std::vector<uint8_t> row;
  for (int i = 0; i < n; i++) {
    int cp = (unsigned char)str[i];
    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&s.file->info, cp, s.scale, s.scale, &x0, &y0, &x1, &y1);
    int gw = x1 - x0, gh = y1 - y0;
    if (gw > 0 && gh > 0) {
      std::vector<uint8_t> bm((size_t)gw * gh);
      stbtt_MakeCodepointBitmap(&s.file->info, bm.data(), gw, gh, gw, s.scale, s.scale, cp);
      for (int y = 0; y < gh; y++)
        blendSpanCoverage(dc, base_y + y0 + y + dc->oy, pen_x + x0 + dc->ox, bm.data() + (size_t)y * gw, gw, pixel);
    }
    pen_x += advancePx(s, cp);
  }
  if (s.underline) {
    int uy = base_y + std::max(1, s.descent / 2);
    fillSpan(dc, uy + dc->oy, x_start + dc->ox, pen_x + dc->ox, pixel);
  }
  touched(dc, x_start + dc->ox - 2, base_y - s.ascent + dc->oy - 2, pen_x + dc->ox + 2, base_y + s.descent + dc->oy + 2);
}

}

using namespace softwin;

BOOL WINAPI GetTextExtentPoint32A(HDC h, LPCSTR s, int n, LPSIZE sz)
{
  DC* dc = D(h);
  if (!dc || !sz)
    return FALSE;
  int a = 0, d = 0;
  int w = textExtent(dc, s ? s : "", s ? n : 0, &a, &d);
  double k = devScale(dc);
  sz->cx = (LONG)std::lround(w / k);
  sz->cy = (LONG)std::lround((a + d) / k);
  return TRUE;
}

BOOL WINAPI GetTextMetricsA(HDC h, LPTEXTMETRICA tm)
{
  DC* dc = D(h);
  memset(tm, 0, sizeof(*tm));
  int a = 0, d = 0, em = 0;
  deviceFontPixels(dc, &a, &d, &em);
  double k = devScale(dc);
  tm->tmAscent = (LONG)std::lround(a / k);
  tm->tmDescent = (LONG)std::lround(d / k);
  tm->tmHeight = tm->tmAscent + tm->tmDescent;
  tm->tmInternalLeading = std::max<LONG>(0, tm->tmHeight - (LONG)std::lround(em / k));
  tm->tmAveCharWidth = (LONG)std::lround(textExtent(dc, "x", 1, nullptr, nullptr) / k);
  tm->tmMaxCharWidth = (LONG)std::lround(textExtent(dc, "W", 1, nullptr, nullptr) / k);
  tm->tmWeight = dc->s.font->lf.lfWeight;
  tm->tmFirstChar = 32;
  tm->tmLastChar = 255;
  tm->tmDefaultChar = '?';
  tm->tmBreakChar = ' ';
  return TRUE;
}

static void textAt(DC* dc, int x, int y, const char* s, int n, const RECT* clip_logical, bool opaque_rect, const RECT* opaque)
{
  int saved = SaveDC((HDC)dc);
  if (clip_logical)
    IntersectClipRect((HDC)dc, clip_logical->left, clip_logical->top, clip_logical->right, clip_logical->bottom);
  int a = 0, d = 0;
  int w = textExtent(dc, s, n, &a, &d);
  double dx, dy;
  devPoint(dc, x, y, dx, dy);
  UINT al = dc->s.text_align;
  if ((al & TA_CENTER) == TA_CENTER)
    dx -= w / 2.0;
  else if (al & TA_RIGHT)
    dx -= w;
  double base;
  if ((al & TA_BASELINE) == TA_BASELINE)
    base = dy;
  else if (al & TA_BOTTOM)
    base = dy - d;
  else
    base = dy + a;
  if (opaque_rect && opaque) {
    HBRUSH b = CreateSolidBrush(dc->s.bk);
    FillRect((HDC)dc, opaque, b);
    DeleteObject(b);
  }
  else if (dc->s.bk_mode == OPAQUE) {
    int x0 = (int)std::lround(dx), y0 = (int)std::lround(base) - a;
    for (int yy = y0; yy < y0 + a + d; yy++)
      fillSpan(dc, yy + dc->oy, x0 + dc->ox, x0 + w + dc->ox, toPixel(dc->s.bk));
  }
  drawTextRun(dc, dx, base, s, n, dc->s.text);
  RestoreDC((HDC)dc, saved);
}

BOOL WINAPI TextOutA(HDC h, int x, int y, LPCSTR s, int n)
{
  DC* dc = D(h);
  if (!dc || !s)
    return FALSE;
  textAt(dc, x, y, s, n, nullptr, false, nullptr);
  return TRUE;
}

BOOL WINAPI ExtTextOutA(HDC h, int x, int y, UINT opt, const RECT* r, LPCSTR s, UINT n, const INT*)
{
  DC* dc = D(h);
  if (!dc)
    return FALSE;
  if ((opt & ETO_OPAQUE) && r && (!s || !n)) {
    HBRUSH b = CreateSolidBrush(dc->s.bk);
    FillRect(h, r, b);
    DeleteObject(b);
    return TRUE;
  }
  if (!s)
    return TRUE;
  int saved_mode = dc->s.bk_mode;
  if (opt & ETO_OPAQUE)
    dc->s.bk_mode = TRANSPARENT;
  textAt(dc, x, y, s, (int)n, (opt & ETO_CLIPPED) ? r : nullptr, (opt & ETO_OPAQUE) != 0, r);
  dc->s.bk_mode = saved_mode;
  return TRUE;
}

static void wrapLines(DC* dc, const char* s, int n, int max_w_dev, bool wrap, std::vector<std::string>& lines)
{
  std::string cur;
  int i = 0;
  while (i < n) {
    char c = s[i];
    if (c == '\r') {
      i++;
      continue;
    }
    if (c == '\n') {
      lines.push_back(cur);
      cur.clear();
      i++;
      continue;
    }
    if (wrap) {
      int j = i;
      while (j < n && s[j] != ' ' && s[j] != '\n')
        j++;
      std::string word(s + i, j - i);
      std::string trial = cur.empty() ? word : cur + " " + word;
      if (!cur.empty() && textExtent(dc, trial.c_str(), (int)trial.size(), nullptr, nullptr) > max_w_dev) {
        lines.push_back(cur);
        cur = word;
      }
      else {
        cur = trial;
      }
      i = j;
      while (i < n && s[i] == ' ')
        i++;
      continue;
    }
    cur.push_back(c);
    i++;
  }
  lines.push_back(cur);
}

int WINAPI DrawTextA(HDC h, LPCSTR s, int n, LPRECT r, UINT fmt)
{
  DC* dc = D(h);
  if (!dc || !s || !r)
    return 0;
  if (n < 0)
    n = (int)strlen(s);
  double k = devScale(dc);
  int a = 0, d = 0;
  deviceFontPixels(dc, &a, &d, nullptr);
  int line_h = a + d;
  std::vector<std::string> lines;
  int box_w_dev = (int)std::lround((r->right - r->left) * k);
  if (fmt & DT_SINGLELINE)
    lines.push_back(std::string(s, n));
  else
    wrapLines(dc, s, n, box_w_dev, (fmt & DT_WORDBREAK) != 0, lines);
  int total_h_dev = line_h * (int)lines.size();
  if (fmt & DT_CALCRECT) {
    int maxw = 0;
    for (auto& l : lines)
      maxw = std::max(maxw, textExtent(dc, l.c_str(), (int)l.size(), nullptr, nullptr));
    if (!(fmt & DT_WORDBREAK) || (fmt & DT_SINGLELINE))
      r->right = r->left + (LONG)std::lround(maxw / k);
    r->bottom = r->top + (LONG)std::lround(total_h_dev / k);
    return (int)std::lround(total_h_dev / k);
  }
  double x0, y0, x1, y1;
  devPoint(dc, r->left, r->top, x0, y0);
  devPoint(dc, r->right, r->bottom, x1, y1);
  int saved = SaveDC(h);
  if (!(fmt & DT_NOCLIP))
    IntersectClipRect(h, r->left, r->top, r->right, r->bottom);
  double ty = y0;
  if (fmt & DT_SINGLELINE) {
    if (fmt & DT_VCENTER)
      ty = y0 + ((y1 - y0) - line_h) / 2.0;
    else if (fmt & DT_BOTTOM)
      ty = y1 - line_h;
  }
  ty = std::floor(ty);
  for (auto& l : lines) {
    int w = textExtent(dc, l.c_str(), (int)l.size(), nullptr, nullptr);
    double tx = x0;
    if (fmt & DT_CENTER)
      tx = std::floor(x0 + ((x1 - x0) - w) / 2.0);
    else if (fmt & DT_RIGHT)
      tx = x1 - w;
    if (dc->s.bk_mode == OPAQUE) {
      int ix = (int)std::lround(tx), iy = (int)std::lround(ty);
      for (int yy = iy; yy < iy + line_h; yy++)
        fillSpan(dc, yy + dc->oy, ix + dc->ox, ix + w + dc->ox, toPixel(dc->s.bk));
    }
    drawTextRun(dc, tx, ty + a, l.c_str(), (int)l.size(), dc->s.text);
    ty += line_h;
  }
  RestoreDC(h, saved);
  return (int)std::lround(total_h_dev / k);
}
