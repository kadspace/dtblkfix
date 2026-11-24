#pragma once

#include <stdint.h>

namespace softwin {

struct SwRect {
  int left, top, right, bottom;
};

struct SwPoint {
  int x, y;
};

enum Modifier {
  kModShift = 1,
  kModControl = 2,
  kModAlt = 4,
  kModCommand = 8,
  kModLButton = 16,
  kModRButton = 32,
  kModMButton = 64
};

enum MouseMessage {
  kMouseMove = 0x0200,
  kLButtonDown = 0x0201,
  kLButtonUp = 0x0202,
  kLButtonDoubleClick = 0x0203,
  kRButtonDown = 0x0204,
  kRButtonUp = 0x0205,
  kMButtonDown = 0x0207,
  kMButtonUp = 0x0208,
  kMouseWheel = 0x020A,
  kMouseHWheel = 0x020E,
  kMouseLeave = 0x02A3
};

enum EditEvent { kEditReturn, kEditEscape, kEditFocusLost };

enum Cursor {
  kCursorArrow = 32512,
  kCursorIBeam = 32513,
  kCursorWait = 32514,
  kCursorCross = 32515,
  kCursorSizeNWSE = 32642,
  kCursorSizeNESW = 32643,
  kCursorSizeWE = 32644,
  kCursorSizeNS = 32645,
  kCursorSizeAll = 32646,
  kCursorNo = 32648,
  kCursorHand = 32649
};

enum MenuFlag {
  kMenuGrayed = 0x1,
  kMenuDisabled = 0x2,
  kMenuChecked = 0x8,
  kMenuPopup = 0x10,
  kMenuSeparator = 0x800
};

struct Host {
  virtual ~Host() {}
  virtual void invalidate(void* root, const SwRect& r) = 0;
  virtual void updateNow(void* root) = 0;
  virtual void setCursor(int cursor) = 0;
  virtual SwPoint rootScreenPos(void* root) = 0;
  virtual SwPoint mouseScreenPos() = 0;
  virtual unsigned modifiers() = 0;
  virtual int popupMenu(void* root, void* menu, int screen_x, int screen_y) = 0;
  virtual void showOverlay(void* wnd, int screen_x, int screen_y, int w, int h, const uint32_t* bgra_premultiplied) = 0;
  virtual void hideOverlay(void* wnd) = 0;
  virtual void textEditOpen(void* edit, const SwRect& in_root, const char* text, int limit, int font_px) = 0;
  virtual void textEditClose(void* edit) = 0;
  virtual int textEditGet(void* edit, char* buf, int n) = 0;
  virtual void textEditSet(void* edit, const char* text) = 0;
};

void setHost(Host* host);
void setFontDirs(const char* const* dirs, int n);
void destroyWindow(void* wnd);
const uint32_t* rootPixels(void* root, int* width, int* height);
void paintPending(void* root);
void mouseEvent(void* root, unsigned message, int x, int y, unsigned modifiers, int wheel_delta);
void textEditEvent(void* edit, EditEvent e);
int menuItemCount(void* menu);
const char* menuItemText(void* menu, int i);
unsigned menuItemFlags(void* menu, int i);
uintptr_t menuItemId(void* menu, int i);

}
