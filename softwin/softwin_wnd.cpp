#include "softwin_internal.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <dirent.h>
#include <dlfcn.h>
#include <fnmatch.h>
#include <mutex>
#include <set>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <cstdarg>
#include <strings.h>

namespace softwin {

static Host* g_host = nullptr;
static std::set<Wnd*> g_windows;
static std::map<std::string, WNDCLASSA> g_classes;
static Wnd* g_focus = nullptr;
static Wnd* g_capture = nullptr;
static HCURSOR g_cursor = nullptr;
static DWORD g_last_error = 0;
static DWORD g_msg_time = 0;
static std::deque<MSG> g_posted;

struct Timer {
  Wnd* wnd;
  UINT_PTR id;
  UINT ms;
  void* proc;
  std::chrono::steady_clock::time_point due;
};
static std::vector<Timer> g_timers;

void setHost(Host* h)
{
  g_host = h;
}

Host* host()
{
  return g_host;
}

Wnd* W(HWND h)
{
  Wnd* w = (Wnd*)h;
  return g_windows.count(w) ? w : nullptr;
}

Wnd* rootOf(Wnd* w)
{
  while (w && w->parent)
    w = w->parent;
  return w;
}

POINT offsetInRoot(Wnd* w)
{
  POINT p = {0, 0};
  while (w && w->parent) {
    p.x += w->rc.left;
    p.y += w->rc.top;
    w = w->parent;
  }
  return p;
}

bool isEdit(Wnd* w)
{
  return w && strcasecmp(w->cls.c_str(), "EDIT") == 0;
}

static int width(Wnd* w)
{
  return w->rc.right - w->rc.left;
}

static int height(Wnd* w)
{
  return w->rc.bottom - w->rc.top;
}

static void presentRect(Wnd* w, const RECT& client)
{
  Wnd* root = rootOf(w);
  if (!root || !g_host || !root->surface)
    return;
  POINT o = offsetInRoot(w);
  SwRect r = {client.left + o.x, client.top + o.y, client.right + o.x, client.bottom + o.y};
  g_host->invalidate((void*)root, r);
}

void invalidateRoot(Wnd* w, const RECT& client)
{
  RECT full = {0, 0, width(w), height(w)};
  RECT r = {std::max(full.left, client.left), std::max(full.top, client.top),
            std::min(full.right, client.right), std::min(full.bottom, client.bottom)};
  if (r.left >= r.right || r.top >= r.bottom)
    return;
  if (w->has_invalid) {
    w->invalid.left = std::min(w->invalid.left, r.left);
    w->invalid.top = std::min(w->invalid.top, r.top);
    w->invalid.right = std::max(w->invalid.right, r.right);
    w->invalid.bottom = std::max(w->invalid.bottom, r.bottom);
  }
  else {
    w->invalid = r;
    w->has_invalid = true;
  }
  presentRect(w, r);
}

static LRESULT send(Wnd* w, UINT msg, WPARAM wp, LPARAM lp)
{
  if (!w || !w->proc || w->destroyed)
    return 0;
  return w->proc((HWND)w, msg, wp, lp);
}

static void collect(Wnd* w, std::vector<Wnd*>& out)
{
  out.push_back(w);
  for (Wnd* c : w->children)
    collect(c, out);
}

static Wnd* hitTest(Wnd* w, int x, int y)
{
  for (auto it = w->children.rbegin(); it != w->children.rend(); ++it) {
    Wnd* c = *it;
    if (!c->visible || isEdit(c) || (c->exstyle & WS_EX_LAYERED))
      continue;
    if (x >= c->rc.left && x < c->rc.right && y >= c->rc.top && y < c->rc.bottom)
      return hitTest(c, x - c->rc.left, y - c->rc.top);
  }
  return w;
}

static unsigned mods()
{
  return g_host ? g_host->modifiers() : 0;
}

static WPARAM mkFlags(unsigned m)
{
  WPARAM f = 0;
  if (m & kModLButton)
    f |= MK_LBUTTON;
  if (m & kModRButton)
    f |= MK_RBUTTON;
  if (m & kModMButton)
    f |= MK_MBUTTON;
  if (m & kModShift)
    f |= MK_SHIFT;
  if (m & (kModControl | kModCommand))
    f |= MK_CONTROL;
  return f;
}

static void editFocus(Wnd* from, Wnd* to)
{
  if (from == to)
    return;
  g_focus = to;
  if (from)
    send(from, WM_KILLFOCUS, (WPARAM)to, 0);
  if (to)
    send(to, WM_SETFOCUS, (WPARAM)from, 0);
}

static LRESULT CALLBACK EditProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
  Wnd* w = W(h);
  if (!w)
    return 0;
  switch (msg) {
    case WM_SETTEXT:
      w->text = lp ? (const char*)lp : "";
      if (w->edit_open && g_host)
        g_host->textEditSet((void*)h, w->text.c_str());
      return TRUE;
    case WM_GETTEXT: {
      if (w->edit_open && g_host) {
        char buf[1024] = {0};
        g_host->textEditGet((void*)h, buf, sizeof(buf));
        w->text = buf;
      }
      int n = std::min<int>((int)w->text.size(), (int)wp - 1);
      if (n < 0)
        return 0;
      memcpy((char*)lp, w->text.data(), n);
      ((char*)lp)[n] = 0;
      return n;
    }
    case WM_GETTEXTLENGTH:
      return (LRESULT)w->text.size();
    case WM_SETFONT:
      w->font = (HFONT)wp;
      return 0;
    case EM_LIMITTEXT:
      w->limit = (int)wp;
      return 0;
    case EM_SETSEL:
    case EM_SETMARGINS:
    case WM_GETDLGCODE:
      return 0;
  }
  return DefWindowProcA(h, msg, wp, lp);
}

static void openEdit(Wnd* w)
{
  if (!g_host || w->edit_open || !w->visible)
    return;
  Wnd* root = rootOf(w);
  POINT o = offsetInRoot(w);
  SwRect r = {o.x, o.y, o.x + width(w), o.y + height(w)};
  int px = 12;
  if (w->font) {
    Font* f = (Font*)w->font;
    px = f->lf.lfHeight < 0 ? -f->lf.lfHeight : (f->lf.lfHeight > 0 ? f->lf.lfHeight * 3 / 4 : 12);
  }
  w->edit_open = true;
  g_host->textEditOpen((void*)w, r, w->text.c_str(), w->limit, px);
  (void)root;
}

static void freeTimersFor(Wnd* w)
{
  g_timers.erase(std::remove_if(g_timers.begin(), g_timers.end(), [&](const Timer& t) { return t.wnd == w; }), g_timers.end());
}

static void destroyTree(Wnd* w)
{
  for (Wnd* c : std::vector<Wnd*>(w->children))
    destroyTree(c);
  w->children.clear();
  send(w, WM_DESTROY, 0, 0);
  send(w, WM_NCDESTROY, 0, 0);
  if (isEdit(w) && w->edit_open && g_host)
    g_host->textEditClose((void*)w);
  if (w->overlay_shown && g_host)
    g_host->hideOverlay((void*)w);
  if (g_focus == w)
    g_focus = nullptr;
  if (g_capture == w)
    g_capture = nullptr;
  freeTimersFor(w);
  w->destroyed = true;
  if (w->parent) {
    auto& v = w->parent->children;
    v.erase(std::remove(v.begin(), v.end(), w), v.end());
  }
  delete w->surface;
  w->surface = nullptr;
  g_windows.erase(w);
  delete w;
}

void destroyWindow(void* wnd)
{
  DestroyWindow((HWND)wnd);
}

static void resizeSurface(Wnd* w)
{
  int ww = std::max(1, width(w)), hh = std::max(1, height(w));
  if (w->surface && w->surface->w == ww && w->surface->h == hh)
    return;
  Bitmap* b = new Bitmap;
  b->w = ww;
  b->h = hh;
  b->bpp = 32;
  b->stride = ww * 4;
  b->bits = (uint8_t*)calloc((size_t)b->stride * hh, 1);
  delete w->surface;
  w->surface = b;
}

const uint32_t* rootPixels(void* root, int* w, int* h)
{
  Wnd* r = W((HWND)root);
  if (!r || !r->surface)
    return nullptr;
  *w = r->surface->w;
  *h = r->surface->h;
  return (const uint32_t*)r->surface->bits;
}

static void paintTree(Wnd* w)
{
  if (!w->visible && w->parent)
    return;
  if (w->has_invalid)
    send(w, WM_PAINT, 0, 0);
  if (w->has_invalid)
    w->has_invalid = false;
  for (Wnd* c : std::vector<Wnd*>(w->children))
    if (g_windows.count(c))
      paintTree(c);
}

static void runTimers()
{
  auto now = std::chrono::steady_clock::now();
  std::vector<Timer> due;
  for (Timer& t : g_timers)
    if (t.due <= now) {
      due.push_back(t);
      t.due = now + std::chrono::milliseconds(std::max<UINT>(t.ms, 1));
    }
  for (const Timer& t : due) {
    if (t.proc)
      ((void (*)(HWND, UINT, UINT_PTR, DWORD))t.proc)((HWND)t.wnd, WM_TIMER, t.id, GetTickCount());
    else if (W((HWND)t.wnd))
      send(t.wnd, WM_TIMER, t.id, 0);
  }
  while (!g_posted.empty()) {
    MSG m = g_posted.front();
    g_posted.pop_front();
    if (W(m.hwnd))
      send(W(m.hwnd), m.message, m.wParam, m.lParam);
  }
}

void paintPending(void* root)
{
  runTimers();
  Wnd* r = W((HWND)root);
  if (r)
    paintTree(r);
}

void mouseEvent(void* root_handle, unsigned msg, int x, int y, unsigned m, int wheel)
{
  HWND root = (HWND)root_handle;
  Wnd* r = W(root);
  if (!r)
    return;
  g_msg_time = GetTickCount();
  Wnd* target = g_capture && rootOf(g_capture) == r ? g_capture : hitTest(r, x, y);
  POINT o = offsetInRoot(target);
  int cx = x - o.x, cy = y - o.y;
  if (msg == WM_MOUSELEAVE) {
    std::vector<Wnd*> all;
    collect(r, all);
    for (Wnd* w : all)
      if (w->track_leave) {
        w->track_leave = false;
        send(w, WM_MOUSELEAVE, 0, 0);
      }
    return;
  }
  if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
    POINT sp = {x, y};
    ClientToScreen(root, &sp);
    send(target, msg, MAKEWPARAM(mkFlags(m), (WORD)(short)wheel), MAKELPARAM(sp.x, sp.y));
    return;
  }
  if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_LBUTTONDBLCLK) {
    if (g_focus && isEdit(g_focus) && g_focus != target)
      editFocus(g_focus, nullptr);
  }
  if (msg == WM_LBUTTONDBLCLK) {
    auto it = g_classes.find(target->cls);
    if (it == g_classes.end() || !(it->second.style & CS_DBLCLKS))
      msg = WM_LBUTTONDOWN;
  }
  send(target, msg, mkFlags(m), MAKELPARAM((WORD)(short)cx, (WORD)(short)cy));
}

void textEditEvent(void* edit, EditEvent e)
{
  Wnd* w = W((HWND)edit);
  if (!w)
    return;
  if (e == kEditFocusLost) {
    if (g_focus == w)
      editFocus(w, nullptr);
    return;
  }
  WPARAM key = e == kEditReturn ? VK_RETURN : VK_ESCAPE;
  send(w, WM_KEYDOWN, key, 0);
  if (W((HWND)edit))
    send(w, WM_CHAR, key == VK_RETURN ? '\r' : 27, 0);
}

int menuItemCount(void* h)
{
  return h ? (int)((Menu*)h)->items.size() : 0;
}

const char* menuItemText(void* h, int i)
{
  return ((Menu*)h)->items[i].text.c_str();
}

unsigned menuItemFlags(void* h, int i)
{
  return ((Menu*)h)->items[i].flags;
}

uintptr_t menuItemId(void* h, int i)
{
  return ((Menu*)h)->items[i].id;
}

std::string unixPath(const char* p)
{
  std::string s = p ? p : "";
  for (char& c : s)
    if (c == '\\')
      c = '/';
  std::string out;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '/' && !out.empty() && out.back() == '/')
      continue;
    out.push_back(s[i]);
  }
  return out;
}

}

using namespace softwin;

FILE* softwin_fopen(const char* path, const char* mode)
{
  return ::fopen(unixPath(path).c_str(), mode);
}

ATOM WINAPI RegisterClassA(const WNDCLASSA* wc)
{
  if (!wc || !wc->lpszClassName)
    return 0;
  g_classes[wc->lpszClassName] = *wc;
  return 1;
}

ATOM WINAPI RegisterClassExA(const WNDCLASSEXA* wc)
{
  WNDCLASSA a;
  memset(&a, 0, sizeof(a));
  a.style = wc->style;
  a.lpfnWndProc = wc->lpfnWndProc;
  a.hInstance = wc->hInstance;
  a.hCursor = wc->hCursor;
  a.hbrBackground = wc->hbrBackground;
  a.lpszClassName = wc->lpszClassName;
  return RegisterClassA(&a);
}

BOOL WINAPI UnregisterClassA(LPCSTR name, HINSTANCE)
{
  return name && g_classes.erase(name) ? TRUE : FALSE;
}

BOOL WINAPI GetClassInfoA(HINSTANCE, LPCSTR name, LPWNDCLASSA out)
{
  auto it = g_classes.find(name ? name : "");
  if (it == g_classes.end())
    return FALSE;
  if (out)
    *out = it->second;
  return TRUE;
}

int WINAPI GetClassNameA(HWND h, LPSTR buf, int n)
{
  Wnd* w = W(h);
  if (!w || n <= 0)
    return 0;
  int k = std::min<int>((int)w->cls.size(), n - 1);
  memcpy(buf, w->cls.data(), k);
  buf[k] = 0;
  return k;
}

HWND WINAPI CreateWindowExA(DWORD ex, LPCSTR cls, LPCSTR name, DWORD style, int x, int y, int w, int h,
                            HWND parent, HMENU menu, HINSTANCE inst, LPVOID param)
{
  Wnd* p = W(parent);
  Wnd* wnd = new Wnd;
  wnd->cls = cls ? cls : "";
  wnd->text = name ? name : "";
  wnd->style = (LONG)style;
  wnd->exstyle = (LONG)ex;
  wnd->inst = inst;
  wnd->id = (LONG_PTR)menu;
  if (x == CW_USEDEFAULT)
    x = 0;
  if (y == CW_USEDEFAULT)
    y = 0;
  if (w == CW_USEDEFAULT)
    w = 100;
  if (h == CW_USEDEFAULT)
    h = 100;
  wnd->rc = {x, y, x + w, y + h};
  if (isEdit(wnd)) {
    wnd->proc = EditProc;
  }
  else {
    auto it = g_classes.find(wnd->cls);
    if (it != g_classes.end()) {
      wnd->proc = it->second.lpfnWndProc;
      wnd->background = it->second.hbrBackground;
    }
    else {
      wnd->proc = DefWindowProcA;
    }
  }
  bool child = (style & WS_CHILD) && p;
  if (child) {
    wnd->parent = p;
    p->children.push_back(wnd);
  }
  else if (!(ex & WS_EX_LAYERED)) {
    resizeSurface(wnd);
  }
  g_windows.insert(wnd);
  wnd->visible = (style & WS_VISIBLE) != 0 || !child;
  CREATESTRUCTA cs;
  memset(&cs, 0, sizeof(cs));
  cs.lpCreateParams = param;
  cs.hInstance = inst;
  cs.hwndParent = parent;
  cs.cx = w;
  cs.cy = h;
  cs.x = x;
  cs.y = y;
  cs.style = (LONG)style;
  cs.lpszName = name;
  cs.lpszClass = cls;
  cs.dwExStyle = ex;
  send(wnd, WM_NCCREATE, 0, (LPARAM)&cs);
  if (send(wnd, WM_CREATE, 0, (LPARAM)&cs) == -1) {
    destroyTree(wnd);
    return NULL;
  }
  if (child && wnd->visible && !isEdit(wnd))
    invalidateRoot(wnd, {0, 0, w, h});
  if (isEdit(wnd) && wnd->visible)
    openEdit(wnd);
  return (HWND)wnd;
}

BOOL WINAPI DestroyWindow(HWND h)
{
  Wnd* w = W(h);
  if (!w)
    return FALSE;
  Wnd* parent = w->parent;
  RECT r = w->rc;
  destroyTree(w);
  if (parent && W((HWND)parent))
    invalidateRoot(parent, r);
  return TRUE;
}

LRESULT WINAPI DefWindowProcA(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
  Wnd* w = W(h);
  switch (msg) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      BeginPaint(h, &ps);
      EndPaint(h, &ps);
      return 0;
    }
    case WM_ERASEBKGND: {
      if (!w || !w->background)
        return 0;
      RECT r = {0, 0, width(w), height(w)};
      FillRect((HDC)wp, &r, w->background);
      return 1;
    }
    case WM_SETTEXT:
      if (w)
        w->text = lp ? (const char*)lp : "";
      return TRUE;
    case WM_GETTEXT: {
      if (!w || wp == 0)
        return 0;
      int n = std::min<int>((int)w->text.size(), (int)wp - 1);
      memcpy((char*)lp, w->text.data(), n);
      ((char*)lp)[n] = 0;
      return n;
    }
    case WM_GETTEXTLENGTH:
      return w ? (LRESULT)w->text.size() : 0;
    case WM_NCCREATE:
      return TRUE;
    case WM_MOUSEWHEEL:
      if (w && w->parent)
        return send(w->parent, msg, wp, lp);
      return 0;
  }
  return 0;
}

LRESULT WINAPI CallWindowProcA(WNDPROC proc, HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
  return proc ? proc(h, msg, wp, lp) : DefWindowProcA(h, msg, wp, lp);
}

LRESULT WINAPI SendMessageA(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
  return send(W(h), msg, wp, lp);
}

BOOL WINAPI PostMessageA(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
  MSG m;
  memset(&m, 0, sizeof(m));
  m.hwnd = h;
  m.message = msg;
  m.wParam = wp;
  m.lParam = lp;
  m.time = GetTickCount();
  g_posted.push_back(m);
  return TRUE;
}

BOOL WINAPI PeekMessageA(LPMSG m, HWND h, UINT lo, UINT hi, UINT flags)
{
  for (auto it = g_posted.begin(); it != g_posted.end(); ++it) {
    if (h && it->hwnd != h)
      continue;
    if ((lo || hi) && (it->message < lo || it->message > hi))
      continue;
    if (m)
      *m = *it;
    if (flags & PM_REMOVE)
      g_posted.erase(it);
    return TRUE;
  }
  return FALSE;
}

BOOL WINAPI GetMessageA(LPMSG m, HWND h, UINT lo, UINT hi)
{
  if (PeekMessageA(m, h, lo, hi, PM_REMOVE))
    return TRUE;
  memset(m, 0, sizeof(*m));
  return TRUE;
}

LRESULT WINAPI DispatchMessageA(const MSG* m)
{
  return m ? send(W(m->hwnd), m->message, m->wParam, m->lParam) : 0;
}

BOOL WINAPI TranslateMessage(const MSG*)
{
  return FALSE;
}

LONG WINAPI GetMessageTime()
{
  return (LONG)g_msg_time;
}

DWORD WINAPI GetMessagePos()
{
  POINT p;
  GetCursorPos(&p);
  return MAKELONG(p.x, p.y);
}

LONG_PTR WINAPI GetWindowLongPtrA(HWND h, int idx)
{
  Wnd* w = W(h);
  if (!w)
    return 0;
  switch (idx) {
    case GWLP_WNDPROC: return (LONG_PTR)w->proc;
    case GWLP_USERDATA: return w->userdata;
    case GWL_STYLE: return w->style;
    case GWL_EXSTYLE: return w->exstyle;
    case GWLP_HINSTANCE: return (LONG_PTR)w->inst;
    case GWLP_ID: return w->id;
  }
  return 0;
}

LONG_PTR WINAPI SetWindowLongPtrA(HWND h, int idx, LONG_PTR v)
{
  Wnd* w = W(h);
  if (!w)
    return 0;
  LONG_PTR prev = GetWindowLongPtrA(h, idx);
  switch (idx) {
    case GWLP_WNDPROC: w->proc = (WNDPROC)v; break;
    case GWLP_USERDATA: w->userdata = v; break;
    case GWL_STYLE: w->style = (LONG)v; break;
    case GWL_EXSTYLE: w->exstyle = (LONG)v; break;
    case GWLP_ID: w->id = v; break;
  }
  return prev;
}

LONG WINAPI GetWindowLongA(HWND h, int idx)
{
  return (LONG)GetWindowLongPtrA(h, idx);
}

LONG WINAPI SetWindowLongA(HWND h, int idx, LONG v)
{
  return (LONG)SetWindowLongPtrA(h, idx, v);
}

BOOL WINAPI SetPropA(HWND h, LPCSTR name, HANDLE v)
{
  Wnd* w = W(h);
  if (!w || !name)
    return FALSE;
  w->props[name] = v;
  return TRUE;
}

HANDLE WINAPI GetPropA(HWND h, LPCSTR name)
{
  Wnd* w = W(h);
  if (!w || !name)
    return NULL;
  auto it = w->props.find(name);
  return it == w->props.end() ? NULL : it->second;
}

HANDLE WINAPI RemovePropA(HWND h, LPCSTR name)
{
  Wnd* w = W(h);
  if (!w || !name)
    return NULL;
  auto it = w->props.find(name);
  if (it == w->props.end())
    return NULL;
  HANDLE v = it->second;
  w->props.erase(it);
  return v;
}

BOOL WINAPI SetWindowTextA(HWND h, LPCSTR s)
{
  return (BOOL)SendMessageA(h, WM_SETTEXT, 0, (LPARAM)s);
}

int WINAPI GetWindowTextA(HWND h, LPSTR buf, int n)
{
  return (int)SendMessageA(h, WM_GETTEXT, (WPARAM)n, (LPARAM)buf);
}

int WINAPI GetWindowTextLengthA(HWND h)
{
  return (int)SendMessageA(h, WM_GETTEXTLENGTH, 0, 0);
}

HWND WINAPI GetParent(HWND h)
{
  Wnd* w = W(h);
  return w ? (HWND)w->parent : NULL;
}

HWND WINAPI SetParent(HWND h, HWND p)
{
  Wnd* w = W(h);
  Wnd* np = W(p);
  if (!w)
    return NULL;
  Wnd* old = w->parent;
  if (old) {
    auto& v = old->children;
    v.erase(std::remove(v.begin(), v.end(), w), v.end());
  }
  w->parent = np;
  if (np)
    np->children.push_back(w);
  return (HWND)old;
}

HWND WINAPI GetAncestor(HWND h, UINT)
{
  return (HWND)rootOf(W(h));
}

HWND WINAPI GetDesktopWindow()
{
  return NULL;
}

BOOL WINAPI IsWindow(HWND h)
{
  return W(h) ? TRUE : FALSE;
}

BOOL WINAPI IsWindowVisible(HWND h)
{
  Wnd* w = W(h);
  return w && w->visible;
}

BOOL WINAPI ShowWindow(HWND h, int cmd)
{
  Wnd* w = W(h);
  if (!w)
    return FALSE;
  bool was = w->visible;
  w->visible = cmd != SW_HIDE;
  if (w->exstyle & WS_EX_LAYERED) {
    if (!w->visible && w->overlay_shown && g_host) {
      g_host->hideOverlay((void*)h);
      w->overlay_shown = false;
    }
    else if (w->visible && !w->overlay.empty() && g_host) {
      g_host->showOverlay((void*)h, w->overlay_pos.x, w->overlay_pos.y, w->overlay_size.cx, w->overlay_size.cy, w->overlay.data());
      w->overlay_shown = true;
    }
  }
  else if (isEdit(w)) {
    if (w->visible)
      openEdit(w);
  }
  else if (w->parent) {
    invalidateRoot(w->parent, w->rc);
  }
  return was;
}

BOOL WINAPI EnableWindow(HWND, BOOL)
{
  return FALSE;
}

BOOL WINAPI GetClientRect(HWND h, LPRECT r)
{
  Wnd* w = W(h);
  if (!w || !r)
    return FALSE;
  *r = {0, 0, width(w), height(w)};
  return TRUE;
}

BOOL WINAPI GetWindowRect(HWND h, LPRECT r)
{
  Wnd* w = W(h);
  if (!w || !r)
    return FALSE;
  POINT p = {0, 0};
  ClientToScreen(h, &p);
  *r = {p.x, p.y, p.x + width(w), p.y + height(w)};
  return TRUE;
}

BOOL WINAPI SetWindowPos(HWND h, HWND, int x, int y, int cx, int cy, UINT flags)
{
  Wnd* w = W(h);
  if (!w)
    return FALSE;
  RECT old = w->rc;
  int nx = (flags & SWP_NOMOVE) ? w->rc.left : x;
  int ny = (flags & SWP_NOMOVE) ? w->rc.top : y;
  int nw = (flags & SWP_NOSIZE) ? width(w) : cx;
  int nh = (flags & SWP_NOSIZE) ? height(w) : cy;
  if (w->exstyle & WS_EX_LAYERED) {
    if (!(flags & SWP_NOMOVE)) {
      w->overlay_pos = {nx, ny};
      if (w->overlay_shown && g_host)
        g_host->showOverlay((void*)h, nx, ny, w->overlay_size.cx, w->overlay_size.cy, w->overlay.data());
    }
    if (flags & SWP_HIDEWINDOW)
      ShowWindow(h, SW_HIDE);
    if (flags & SWP_SHOWWINDOW)
      ShowWindow(h, SW_SHOW);
    return TRUE;
  }
  w->rc = {nx, ny, nx + nw, ny + nh};
  if (flags & SWP_HIDEWINDOW)
    w->visible = false;
  if (flags & SWP_SHOWWINDOW)
    w->visible = true;
  if (!w->parent) {
    resizeSurface(w);
    if (w->surface)
      invalidateRoot(w, {0, 0, nw, nh});
  }
  else {
    invalidateRoot(w->parent, old);
    invalidateRoot(w->parent, w->rc);
  }
  if (isEdit(w) && w->edit_open && g_host) {
    g_host->textEditClose((void*)h);
    w->edit_open = false;
    openEdit(w);
  }
  if (!(flags & SWP_NOSIZE))
    send(w, WM_SIZE, 0, MAKELPARAM(nw, nh));
  return TRUE;
}

BOOL WINAPI MoveWindow(HWND h, int x, int y, int cx, int cy, BOOL)
{
  return SetWindowPos(h, NULL, x, y, cx, cy, SWP_NOZORDER);
}

BOOL WINAPI ClientToScreen(HWND h, LPPOINT p)
{
  Wnd* w = W(h);
  if (!w || !p)
    return FALSE;
  if (w->exstyle & WS_EX_LAYERED) {
    p->x += w->overlay_pos.x;
    p->y += w->overlay_pos.y;
    return TRUE;
  }
  POINT o = offsetInRoot(w);
  Wnd* root = rootOf(w);
  SwPoint sp = g_host ? g_host->rootScreenPos((void*)root) : SwPoint{0, 0};
  POINT rs = {sp.x, sp.y};
  p->x += o.x + rs.x;
  p->y += o.y + rs.y;
  return TRUE;
}

BOOL WINAPI ScreenToClient(HWND h, LPPOINT p)
{
  POINT z = {0, 0};
  if (!ClientToScreen(h, &z))
    return FALSE;
  p->x -= z.x;
  p->y -= z.y;
  return TRUE;
}

int WINAPI MapWindowPoints(HWND from, HWND to, LPPOINT p, UINT n)
{
  POINT a = {0, 0}, b = {0, 0};
  if (from)
    ClientToScreen(from, &a);
  if (to)
    ClientToScreen(to, &b);
  for (UINT i = 0; i < n; i++) {
    p[i].x += a.x - b.x;
    p[i].y += a.y - b.y;
  }
  return MAKELONG(a.x - b.x, a.y - b.y);
}

HWND WINAPI WindowFromPoint(POINT)
{
  return NULL;
}

BOOL WINAPI InvalidateRect(HWND h, const RECT* r, BOOL erase)
{
  Wnd* w = W(h);
  if (!w)
    return FALSE;
  RECT full = {0, 0, width(w), height(w)};
  invalidateRoot(w, r ? *r : full);
  if (erase)
    w->erase = true;
  return TRUE;
}

BOOL WINAPI ValidateRect(HWND h, const RECT*)
{
  Wnd* w = W(h);
  if (w)
    w->has_invalid = false;
  return TRUE;
}

BOOL WINAPI InvalidateRgn(HWND h, HRGN rgn, BOOL erase)
{
  if (!rgn)
    return InvalidateRect(h, NULL, erase);
  RECT b;
  GetRgnBox(rgn, &b);
  return InvalidateRect(h, &b, erase);
}

BOOL WINAPI RedrawWindow(HWND h, const RECT* r, HRGN rgn, UINT flags)
{
  if (flags & RDW_INVALIDATE)
    rgn ? InvalidateRgn(h, rgn, FALSE) : InvalidateRect(h, r, FALSE);
  if (flags & RDW_UPDATENOW)
    UpdateWindow(h);
  return TRUE;
}

BOOL WINAPI UpdateWindow(HWND h)
{
  Wnd* w = W(h);
  if (!w)
    return FALSE;
  if (w->has_invalid)
    send(w, WM_PAINT, 0, 0);
  w->has_invalid = false;
  Wnd* root = rootOf(w);
  if (g_host && root && root->surface)
    g_host->updateNow((void*)root);
  return TRUE;
}

BOOL WINAPI GetUpdateRect(HWND h, LPRECT r, BOOL)
{
  Wnd* w = W(h);
  if (!w || !w->has_invalid) {
    if (r)
      *r = {0, 0, 0, 0};
    return FALSE;
  }
  if (r)
    *r = w->invalid;
  return TRUE;
}

int WINAPI GetUpdateRgn(HWND h, HRGN rgn, BOOL)
{
  Wnd* w = W(h);
  Region* g = (Region*)rgn;
  g->rects.clear();
  if (!w || !w->has_invalid)
    return NULLREGION;
  g->rects.push_back(w->invalid);
  return SIMPLEREGION;
}

static DC* windowDC(Wnd* w)
{
  DC* dc = new DC;
  dc->wnd = w;
  dc->window_dc = true;
  Wnd* root = rootOf(w);
  if (root && root->surface) {
    POINT o = offsetInRoot(w);
    dc->ox = o.x;
    dc->oy = o.y;
    dc->s.bmp = root->surface;
    dc->wnd_area = {o.x, o.y, o.x + width(w), o.y + height(w)};
  }
  return dc;
}

HDC WINAPI GetDC(HWND h)
{
  if (!h)
    return screenDC();
  Wnd* w = W(h);
  if (!w)
    return NULL;
  return (HDC)windowDC(w);
}

HDC WINAPI GetWindowDC(HWND h)
{
  return GetDC(h);
}

int WINAPI ReleaseDC(HWND, HDC h)
{
  DC* dc = h ? D(h) : nullptr;
  if (!dc)
    return 0;
  delete dc;
  return 1;
}

HDC WINAPI BeginPaint(HWND h, LPPAINTSTRUCT ps)
{
  Wnd* w = W(h);
  if (!w)
    return NULL;
  DC* dc = windowDC(w);
  dc->painting = true;
  dc->paint_rect = w->has_invalid ? w->invalid : RECT{0, 0, width(w), height(w)};
  memset(ps, 0, sizeof(*ps));
  ps->hdc = (HDC)dc;
  ps->rcPaint = dc->paint_rect;
  ps->fErase = w->erase;
  if (w->erase) {
    w->erase = false;
    send(w, WM_ERASEBKGND, (WPARAM)dc, 0);
  }
  w->has_invalid = false;
  return (HDC)dc;
}

BOOL WINAPI EndPaint(HWND h, const PAINTSTRUCT* ps)
{
  Wnd* w = W(h);
  if (w && ps)
    presentRect(w, ps->rcPaint);
  if (ps && ps->hdc)
    delete D(ps->hdc);
  return TRUE;
}

BOOL WINAPI ScrollWindow(HWND h, int, int, const RECT* r, const RECT*)
{
  return InvalidateRect(h, r, FALSE);
}

int WINAPI SetWindowRgn(HWND h, HRGN rgn, BOOL)
{
  Wnd* w = W(h);
  if (w) {
    w->has_region = rgn != NULL;
    w->region = rgn ? ((Region*)rgn)->rects : std::vector<RECT>();
  }
  if (rgn)
    DeleteObject(rgn);
  return 1;
}

int WINAPI GetWindowRgn(HWND h, HRGN rgn)
{
  Wnd* w = W(h);
  if (!w || !w->has_region || !rgn)
    return ERROR;
  ((Region*)rgn)->rects = w->region;
  return w->region.empty() ? NULLREGION : (w->region.size() == 1 ? SIMPLEREGION : COMPLEXREGION);
}

HWND WINAPI GetWindow(HWND h, UINT cmd)
{
  Wnd* w = W(h);
  if (!w)
    return NULL;
  if (cmd == GW_CHILD)
    return w->children.empty() ? NULL : (HWND)w->children.front();
  if ((cmd == GW_HWNDNEXT || cmd == GW_HWNDPREV) && w->parent) {
    auto& v = w->parent->children;
    auto it = std::find(v.begin(), v.end(), w);
    if (cmd == GW_HWNDNEXT)
      return (it != v.end() && it + 1 != v.end()) ? (HWND) * (it + 1) : NULL;
    return (it != v.begin()) ? (HWND) * (it - 1) : NULL;
  }
  return NULL;
}

HWND WINAPI FindWindowExA(HWND parent, HWND after, LPCSTR cls, LPCSTR title)
{
  Wnd* p = W(parent);
  if (!p)
    return NULL;
  bool started = after == NULL;
  for (Wnd* c : p->children) {
    if (!started) {
      started = (HWND)c == after;
      continue;
    }
    if (cls && strcasecmp(c->cls.c_str(), cls) != 0)
      continue;
    if (title && c->text != title)
      continue;
    return (HWND)c;
  }
  return NULL;
}

BOOL WINAPI UpdateLayeredWindow(HWND h, HDC, POINT* dst, SIZE* size, HDC src, POINT* src_pt, COLORREF, BLENDFUNCTION* blend, DWORD)
{
  Wnd* w = W(h);
  if (!w)
    return FALSE;
  if (dst)
    w->overlay_pos = *dst;
  if (size)
    w->overlay_size = *size;
  DC* s = D(src);
  if (s && s->surface() && size) {
    Bitmap* b = s->surface();
    int sx = src_pt ? src_pt->x : 0, sy = src_pt ? src_pt->y : 0;
    w->overlay.assign((size_t)size->cx * size->cy, 0);
    uint32_t ca = blend ? blend->SourceConstantAlpha : 255;
    bool per_pixel = blend && (blend->AlphaFormat & AC_SRC_ALPHA);
    for (int y = 0; y < size->cy; y++)
      for (int x = 0; x < size->cx; x++) {
        int px = sx + x, py = sy + y;
        if (px < 0 || py < 0 || px >= b->w || py >= b->h)
          continue;
        uint32_t v = b->get(px, py);
        if (!per_pixel)
          v = (v & 0x00ffffffu) | 0xff000000u;
        uint32_t c[4];
        for (int k = 0; k < 4; k++)
          c[k] = (((v >> (8 * k)) & 0xff) * ca + 127) / 255;
        w->overlay[(size_t)y * size->cx + x] = c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);
      }
  }
  if (g_host && w->visible) {
    g_host->showOverlay((void*)h, w->overlay_pos.x, w->overlay_pos.y, w->overlay_size.cx, w->overlay_size.cy, w->overlay.data());
    w->overlay_shown = true;
  }
  return TRUE;
}

BOOL WINAPI SetLayeredWindowAttributes(HWND, COLORREF, BYTE, DWORD)
{
  return TRUE;
}

HWND WINAPI SetFocus(HWND h)
{
  Wnd* prev = g_focus;
  Wnd* w = W(h);
  editFocus(prev, w);
  if (w && isEdit(w))
    openEdit(w);
  return (HWND)prev;
}

HWND WINAPI GetFocus()
{
  return (HWND)g_focus;
}

HWND WINAPI SetCapture(HWND h)
{
  Wnd* prev = g_capture;
  g_capture = W(h);
  return (HWND)prev;
}

HWND WINAPI GetCapture()
{
  return (HWND)g_capture;
}

BOOL WINAPI ReleaseCapture()
{
  Wnd* prev = g_capture;
  g_capture = nullptr;
  if (prev)
    send(prev, WM_CAPTURECHANGED, 0, 0);
  return TRUE;
}

HWND WINAPI GetActiveWindow()
{
  return NULL;
}

HWND WINAPI GetForegroundWindow()
{
  return NULL;
}

BOOL WINAPI SetForegroundWindow(HWND)
{
  return TRUE;
}

BOOL WINAPI TrackMouseEvent(LPTRACKMOUSEEVENT t)
{
  Wnd* w = t ? W(t->hwndTrack) : nullptr;
  if (w && (t->dwFlags & TME_LEAVE))
    w->track_leave = true;
  return TRUE;
}

UINT_PTR WINAPI SetTimer(HWND h, UINT_PTR id, UINT ms, void* proc)
{
  static UINT_PTR next = 1000;
  Wnd* w = W(h);
  if (!w)
    id = next++;
  for (Timer& t : g_timers)
    if (t.wnd == w && t.id == id) {
      t.ms = ms;
      t.proc = proc;
      t.due = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
      return id;
    }
  g_timers.push_back({w, id, ms, proc, std::chrono::steady_clock::now() + std::chrono::milliseconds(ms)});
  return id;
}

BOOL WINAPI KillTimer(HWND h, UINT_PTR id)
{
  Wnd* w = W(h);
  size_t n = g_timers.size();
  g_timers.erase(std::remove_if(g_timers.begin(), g_timers.end(), [&](const Timer& t) { return t.wnd == w && t.id == id; }), g_timers.end());
  return n != g_timers.size();
}

HCURSOR WINAPI LoadCursorA(HINSTANCE, LPCSTR name)
{
  return (HCURSOR)name;
}

HCURSOR WINAPI SetCursor(HCURSOR c)
{
  HCURSOR prev = g_cursor;
  g_cursor = c;
  if (g_host)
    g_host->setCursor(c ? (int)(uintptr_t)c : 0);
  return prev;
}

HCURSOR WINAPI GetCursor()
{
  return g_cursor;
}

BOOL WINAPI GetCursorPos(LPPOINT p)
{
  if (!p)
    return FALSE;
  SwPoint m = g_host ? g_host->mouseScreenPos() : SwPoint{0, 0};
  *p = {m.x, m.y};
  return TRUE;
}

BOOL WINAPI SetCursorPos(int, int)
{
  return TRUE;
}

int WINAPI ShowCursor(BOOL show)
{
  return show ? 0 : -1;
}

SHORT WINAPI GetAsyncKeyState(int vk)
{
  unsigned m = mods();
  bool down = false;
  switch (vk) {
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT: down = m & kModShift; break;
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL: down = m & (kModControl | kModCommand); break;
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU: down = m & kModAlt; break;
    case VK_LBUTTON: down = m & kModLButton; break;
    case VK_RBUTTON: down = m & kModRButton; break;
    case VK_MBUTTON: down = m & kModMButton; break;
  }
  return down ? (SHORT)0x8000 : 0;
}

SHORT WINAPI GetKeyState(int vk)
{
  return GetAsyncKeyState(vk);
}

UINT WINAPI GetDoubleClickTime()
{
  return 500;
}

int WINAPI GetSystemMetrics(int i)
{
  switch (i) {
    case SM_CXSCREEN:
    case SM_CXVIRTUALSCREEN: return 3840;
    case SM_CYSCREEN:
    case SM_CYVIRTUALSCREEN: return 2160;
    case SM_CXDOUBLECLK:
    case SM_CYDOUBLECLK:
    case SM_CXDRAG:
    case SM_CYDRAG: return 4;
    case SM_CXVSCROLL:
    case SM_CYHSCROLL: return 15;
    case SM_CMOUSEBUTTONS: return 3;
  }
  return 0;
}

DWORD WINAPI GetSysColor(int i)
{
  switch (i) {
    case COLOR_MENU:
    case COLOR_BTNFACE:
    case COLOR_INFOBK: return RGB(240, 240, 240);
    case COLOR_MENUTEXT:
    case COLOR_BTNTEXT:
    case COLOR_WINDOWTEXT:
    case COLOR_INFOTEXT: return RGB(0, 0, 0);
    case COLOR_WINDOW: return RGB(255, 255, 255);
    case COLOR_HIGHLIGHT: return RGB(0, 120, 215);
    case COLOR_HIGHLIGHTTEXT: return RGB(255, 255, 255);
    case COLOR_GRAYTEXT: return RGB(109, 109, 109);
    case COLOR_3DSHADOW: return RGB(160, 160, 160);
    case COLOR_3DHILIGHT: return RGB(255, 255, 255);
  }
  return RGB(192, 192, 192);
}

BOOL WINAPI MessageBeep(UINT)
{
  return TRUE;
}

int WINAPI MessageBoxA(HWND, LPCSTR text, LPCSTR title, UINT)
{
  fprintf(stderr, "DtBlkFix: %s: %s\n", title ? title : "", text ? text : "");
  return IDOK;
}

HMENU WINAPI CreatePopupMenu()
{
  return (HMENU) new Menu;
}

HMENU WINAPI CreateMenu()
{
  return CreatePopupMenu();
}

BOOL WINAPI DestroyMenu(HMENU h)
{
  Menu* m = (Menu*)h;
  if (!m)
    return FALSE;
  for (auto& it : m->items)
    if (it.flags & MF_POPUP)
      DestroyMenu((HMENU)it.id);
  delete m;
  return TRUE;
}

BOOL WINAPI AppendMenuA(HMENU h, UINT flags, UINT_PTR id, LPCSTR text)
{
  Menu* m = (Menu*)h;
  if (!m)
    return FALSE;
  MenuItem it;
  it.flags = flags;
  it.id = id;
  it.text = (text && !(flags & (MF_SEPARATOR | MF_BITMAP | MF_OWNERDRAW))) ? text : "";
  m->items.push_back(it);
  return TRUE;
}

BOOL WINAPI InsertMenuA(HMENU h, UINT pos, UINT flags, UINT_PTR id, LPCSTR text)
{
  Menu* m = (Menu*)h;
  if (!m)
    return FALSE;
  MenuItem it;
  it.flags = flags & ~MF_BYPOSITION;
  it.id = id;
  it.text = text && !(flags & MF_SEPARATOR) ? text : "";
  if ((flags & MF_BYPOSITION) && pos <= m->items.size())
    m->items.insert(m->items.begin() + pos, it);
  else
    m->items.push_back(it);
  return TRUE;
}

DWORD WINAPI CheckMenuItem(HMENU h, UINT item, UINT flags)
{
  Menu* m = (Menu*)h;
  if (!m)
    return (DWORD)-1;
  for (size_t i = 0; i < m->items.size(); i++) {
    bool hit = (flags & MF_BYPOSITION) ? i == item : m->items[i].id == item;
    if (hit) {
      DWORD prev = m->items[i].flags & MF_CHECKED;
      m->items[i].flags = (m->items[i].flags & ~MF_CHECKED) | (flags & MF_CHECKED);
      return prev;
    }
  }
  return (DWORD)-1;
}

BOOL WINAPI EnableMenuItem(HMENU h, UINT item, UINT flags)
{
  Menu* m = (Menu*)h;
  if (!m)
    return -1;
  for (size_t i = 0; i < m->items.size(); i++) {
    bool hit = (flags & MF_BYPOSITION) ? i == item : m->items[i].id == item;
    if (hit)
      m->items[i].flags = (m->items[i].flags & ~(MF_GRAYED | MF_DISABLED)) | (flags & (MF_GRAYED | MF_DISABLED));
  }
  return 0;
}

int WINAPI GetMenuItemCount(HMENU h)
{
  return menuItemCount((void*)h);
}

BOOL WINAPI TrackPopupMenu(HMENU h, UINT flags, int x, int y, int, HWND owner, const RECT*)
{
  Wnd* w = W(owner);
  Wnd* root = rootOf(w);
  int id = g_host ? g_host->popupMenu((void*)root, (void*)h, x, y) : 0;
  if (flags & TPM_RETURNCMD)
    return id;
  if (id && w)
    send(w, WM_COMMAND, (WPARAM)id, 0);
  return id != 0;
}

DWORD WINAPI GetTickCount()
{
  using namespace std::chrono;
  return (DWORD)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void WINAPI Sleep(DWORD ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

DWORD WINAPI GetLastError()
{
  return g_last_error;
}

void WINAPI SetLastError(DWORD e)
{
  g_last_error = e;
}

BOOL WINAPI GetVersionExA(LPOSVERSIONINFOA v)
{
  if (!v)
    return FALSE;
  v->dwMajorVersion = 10;
  v->dwMinorVersion = 0;
  v->dwBuildNumber = 19045;
  v->dwPlatformId = VER_PLATFORM_WIN32_NT;
  v->szCSDVersion[0] = 0;
  return TRUE;
}

static int g_lib_token;

HMODULE WINAPI LoadLibraryA(LPCSTR name)
{
  if (!name)
    return NULL;
  std::string n = name;
  for (char& c : n)
    c = (char)tolower((unsigned char)c);
  if (n.find("msimg32") != std::string::npos || n.find("user32") != std::string::npos || n.find("gdi32") != std::string::npos)
    return (HMODULE)&g_lib_token;
  return NULL;
}

BOOL WINAPI FreeLibrary(HMODULE)
{
  return TRUE;
}

FARPROC WINAPI GetProcAddress(HMODULE m, LPCSTR name)
{
  if (m != (HMODULE)&g_lib_token || !name)
    return NULL;
  if (!strcmp(name, "AlphaBlend"))
    return (FARPROC)(void*)&AlphaBlend;
  if (!strcmp(name, "TransparentBlt"))
    return (FARPROC)(void*)&TransparentBlt;
  if (!strcmp(name, "UpdateLayeredWindow"))
    return (FARPROC)(void*)&UpdateLayeredWindow;
  if (!strcmp(name, "SetLayeredWindowAttributes"))
    return (FARPROC)(void*)&SetLayeredWindowAttributes;
  return NULL;
}

HMODULE WINAPI GetModuleHandleA(LPCSTR)
{
  return (HMODULE)&g_lib_token;
}

DWORD WINAPI GetModuleFileNameA(HMODULE, LPSTR buf, DWORD n)
{
  Dl_info info;
  if (!buf || n == 0 || !dladdr((void*)&GetModuleFileNameA, &info) || !info.dli_fname)
    return 0;
  strncpy(buf, info.dli_fname, n - 1);
  buf[n - 1] = 0;
  return (DWORD)strlen(buf);
}

int WINAPI MultiByteToWideChar(UINT, DWORD, LPCSTR s, int n, LPWSTR out, int cap)
{
  if (!s)
    return 0;
  if (n < 0)
    n = (int)strlen(s) + 1;
  if (!out || cap == 0)
    return n;
  int k = std::min(n, cap);
  for (int i = 0; i < k; i++)
    out[i] = (unsigned char)s[i];
  return k;
}

int WINAPI WideCharToMultiByte(UINT, DWORD, LPCWSTR s, int n, LPSTR out, int cap, LPCSTR, LPBOOL)
{
  if (!s)
    return 0;
  if (n < 0)
    n = (int)wcslen(s) + 1;
  if (!out || cap == 0)
    return n;
  int k = std::min(n, cap);
  for (int i = 0; i < k; i++)
    out[i] = s[i] < 256 ? (char)s[i] : '?';
  return k;
}

HGLOBAL WINAPI GlobalAlloc(UINT, size_t n)
{
  size_t* p = (size_t*)calloc(1, n + sizeof(size_t));
  if (!p)
    return NULL;
  *p = n;
  return p + 1;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL h)
{
  if (h)
    free((size_t*)h - 1);
  return NULL;
}

LPVOID WINAPI GlobalLock(HGLOBAL h)
{
  return h;
}

BOOL WINAPI GlobalUnlock(HGLOBAL)
{
  return TRUE;
}

size_t WINAPI GlobalSize(HGLOBAL h)
{
  return h ? *((size_t*)h - 1) : 0;
}

UINT WINAPI DragQueryFileA(HDROP, UINT, LPSTR, UINT)
{
  return 0;
}

void WINAPI DragFinish(HDROP)
{
}

void WINAPI DragAcceptFiles(HWND, BOOL)
{
}

HRESULT WINAPI SHGetFolderPathA(HWND, int csidl, HANDLE, DWORD, LPSTR out)
{
  const char* home = getenv("HOME");
  if (!home || !out)
    return E_FAIL;
  std::string p = home;
  switch (csidl & 0xff) {
    case CSIDL_PERSONAL: p += "/Documents"; break;
    case CSIDL_APPDATA:
    case CSIDL_LOCAL_APPDATA:
#if defined(__APPLE__)
      p += "/Library/Application Support";
#else
      p += "/.local/share";
#endif
      break;
  }
  strncpy(out, p.c_str(), MAX_PATH - 1);
  out[MAX_PATH - 1] = 0;
  return S_OK;
}

BOOL WINAPI CreateDirectoryA(LPCSTR p, void*)
{
  return mkdir(unixPath(p).c_str(), 0755) == 0;
}

DWORD WINAPI GetFileAttributesA(LPCSTR p)
{
  struct stat st;
  if (stat(unixPath(p).c_str(), &st) != 0)
    return INVALID_FILE_ATTRIBUTES;
  return S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}

struct FindState {
  DIR* dir;
  std::string base, pattern;
};

static bool nextMatch(FindState* f, LPWIN32_FIND_DATAA fd)
{
  while (struct dirent* e = readdir(f->dir)) {
    if (fnmatch(f->pattern.c_str(), e->d_name, FNM_CASEFOLD) != 0)
      continue;
    memset(fd, 0, sizeof(*fd));
    strncpy(fd->cFileName, e->d_name, MAX_PATH - 1);
    struct stat st;
    std::string full = f->base + "/" + e->d_name;
    if (stat(full.c_str(), &st) == 0) {
      fd->dwFileAttributes = S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
      fd->nFileSizeLow = (DWORD)st.st_size;
    }
    return true;
  }
  return false;
}

HANDLE WINAPI FindFirstFileA(LPCSTR spec, LPWIN32_FIND_DATAA fd)
{
  std::string s = unixPath(spec);
  size_t slash = s.find_last_of('/');
  FindState* f = new FindState;
  f->base = slash == std::string::npos ? "." : s.substr(0, slash);
  f->pattern = slash == std::string::npos ? s : s.substr(slash + 1);
  if (f->base.empty())
    f->base = "/";
  f->dir = opendir(f->base.c_str());
  if (!f->dir || !nextMatch(f, fd)) {
    if (f->dir)
      closedir(f->dir);
    delete f;
    return INVALID_HANDLE_VALUE;
  }
  return (HANDLE)f;
}

BOOL WINAPI FindNextFileA(HANDLE h, LPWIN32_FIND_DATAA fd)
{
  if (!h || h == INVALID_HANDLE_VALUE)
    return FALSE;
  return nextMatch((FindState*)h, fd);
}

BOOL WINAPI FindClose(HANDLE h)
{
  if (!h || h == INVALID_HANDLE_VALUE)
    return FALSE;
  FindState* f = (FindState*)h;
  closedir(f->dir);
  delete f;
  return TRUE;
}

HANDLE WINAPI CreateFileA(LPCSTR path, DWORD access, DWORD, void*, DWORD disp, DWORD, HANDLE)
{
  const char* mode = (access & GENERIC_WRITE) ? (disp == OPEN_EXISTING || disp == OPEN_ALWAYS ? "r+b" : "wb") : "rb";
  FILE* f = ::fopen(unixPath(path).c_str(), mode);
  return f ? (HANDLE)f : INVALID_HANDLE_VALUE;
}

BOOL WINAPI WriteFile(HANDLE h, LPCVOID data, DWORD n, LPDWORD written, void*)
{
  size_t k = fwrite(data, 1, n, (FILE*)h);
  if (written)
    *written = (DWORD)k;
  return k == n;
}

BOOL WINAPI ReadFile(HANDLE h, LPVOID data, DWORD n, LPDWORD got, void*)
{
  size_t k = fread(data, 1, n, (FILE*)h);
  if (got)
    *got = (DWORD)k;
  return TRUE;
}

BOOL WINAPI CloseHandle(HANDLE h)
{
  if (!h || h == INVALID_HANDLE_VALUE)
    return FALSE;
  return ::fclose((FILE*)h) == 0;
}

BOOL WINAPI DeleteFileA(LPCSTR p)
{
  return unlink(unixPath(p).c_str()) == 0;
}

BOOL WINAPI RemoveDirectoryA(LPCSTR p)
{
  return rmdir(unixPath(p).c_str()) == 0;
}

BOOL WINAPI MoveFileA(LPCSTR a, LPCSTR b)
{
  return rename(unixPath(a).c_str(), unixPath(b).c_str()) == 0;
}

DWORD WINAPI GetEnvironmentVariableA(LPCSTR name, LPSTR buf, DWORD n)
{
  const char* v = name ? getenv(name) : nullptr;
  if (!v)
    return 0;
  size_t len = strlen(v);
  if (!buf || n <= len)
    return (DWORD)len + 1;
  memcpy(buf, v, len + 1);
  return (DWORD)len;
}

DWORD WINAPI GetCurrentProcessId()
{
  return (DWORD)getpid();
}

DWORD WINAPI GetCurrentThreadId()
{
  return (DWORD)(std::hash<std::thread::id>()(std::this_thread::get_id()) & 0xffffffff);
}

BOOL WINAPI QueryPerformanceCounter(LONGLONG* v)
{
  using namespace std::chrono;
  *v = (LONGLONG)duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
  return TRUE;
}

BOOL WINAPI QueryPerformanceFrequency(LONGLONG* v)
{
  *v = 1000000000LL;
  return TRUE;
}

void WINAPI OutputDebugStringA(LPCSTR)
{
}

HRESULT WINAPI OleInitialize(LPVOID)
{
  return S_OK;
}

void WINAPI OleUninitialize()
{
}

HRESULT WINAPI CoInitialize(LPVOID)
{
  return S_OK;
}

void WINAPI CoUninitialize()
{
}

HRESULT WINAPI CoCreateInstance(REFCLSID, void*, DWORD, REFIID, LPVOID* out)
{
  if (out)
    *out = NULL;
  return E_NOTIMPL;
}

HRESULT WINAPI RegisterDragDrop(HWND, IDropTarget*)
{
  return S_OK;
}

HRESULT WINAPI RevokeDragDrop(HWND)
{
  return S_OK;
}

int _vsnprintf_s(char* buf, size_t size, size_t count, const char* fmt, va_list args)
{
  if (!buf || size == 0)
    return -1;
  size_t lim = count == _TRUNCATE ? size : std::min(size, count + 1);
  int n = vsnprintf(buf, lim, fmt, args);
  if (n < 0 || (size_t)n >= lim)
    return -1;
  return n;
}

int _snprintf_s(char* buf, size_t size, size_t count, const char* fmt, ...)
{
  va_list a;
  va_start(a, fmt);
  int n = _vsnprintf_s(buf, size, count, fmt, a);
  va_end(a);
  return n;
}

int sprintf_s(char* buf, size_t size, const char* fmt, ...)
{
  va_list a;
  va_start(a, fmt);
  int n = vsnprintf(buf, size, fmt, a);
  va_end(a);
  return n;
}

int strcpy_s(char* dst, size_t size, const char* src)
{
  if (!dst || !size)
    return 22;
  strncpy(dst, src ? src : "", size - 1);
  dst[size - 1] = 0;
  return 0;
}

int strncpy_s(char* dst, size_t size, const char* src, size_t n)
{
  if (!dst || !size)
    return 22;
  size_t k = std::min(size - 1, n);
  strncpy(dst, src ? src : "", k);
  dst[k] = 0;
  return 0;
}

int strcat_s(char* dst, size_t size, const char* src)
{
  size_t len = strlen(dst);
  if (len >= size)
    return 22;
  strncat(dst, src ? src : "", size - len - 1);
  return 0;
}

int _stricmp(const char* a, const char* b)
{
  return strcasecmp(a, b);
}

int _strnicmp(const char* a, const char* b, size_t n)
{
  return strncasecmp(a, b, n);
}

void WINAPI ReleaseStgMedium(STGMEDIUM* m)
{
  if (m && m->hGlobal)
    GlobalFree(m->hGlobal);
}

DWORD WINAPI GetTempPathA(DWORD n, LPSTR buf)
{
  const char* t = getenv("TMPDIR");
  std::string p = t && *t ? t : "/tmp";
  if (p.back() != '/')
    p += '/';
  if (buf && n > p.size())
    memcpy(buf, p.c_str(), p.size() + 1);
  return (DWORD)p.size();
}

BOOL WINAPI SetEnvironmentVariableA(LPCSTR name, LPCSTR value)
{
  if (!name)
    return FALSE;
  if (value)
    return setenv(name, value, 1) == 0;
  return unsetenv(name) == 0;
}

int _putenv_s(const char* name, const char* value)
{
  return SetEnvironmentVariableA(name, value) ? 0 : 22;
}

BOOL WINAPI AdjustWindowRect(LPRECT, DWORD, BOOL)
{
  return TRUE;
}
