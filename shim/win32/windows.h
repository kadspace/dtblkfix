#ifndef _DTBLKFIX_WIN32_SHIM_H_
#define _DTBLKFIX_WIN32_SHIM_H_
/**************************************************************************************************
DtBlkFix Win32 shim

A small stand-in for the parts of the Win32 API (windows, messages, GDI, files) that the DtBlkFx
engine, its editor and VSTGUI 3.5 use, so that they build and run where there is no Windows. It
draws in software into 32 bit bitmaps, and the wrapper shows a window's pixels and feeds it mouse
and keyboard events through win32host.h. It was written from the way the code in this repo calls
the API and from the API's public documentation, not from Microsoft's headers, and nothing here
is binary compatible with Windows.

This program is free software; you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

***************************************************************************************************/

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DTBLKFIX_WIN32_SHIM 1
#define STRICT 1

#ifndef NULL
#  define NULL 0
#endif
#ifndef FALSE
#  define FALSE 0
#endif
#ifndef TRUE
#  define TRUE 1
#endif

#define WINAPI
#define CALLBACK
#define APIENTRY
#define PASCAL
#define CONST const
#define FAR
#define NEAR
#define IN
#define OUT
#ifndef _MSC_VER
#  define __cdecl
#  define __stdcall
#endif

typedef int BOOL;
typedef int INT;
typedef unsigned int UINT;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int16_t SHORT;
typedef uint16_t USHORT;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;
typedef float FLOAT;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef wchar_t WCHAR;
typedef char TCHAR;
typedef void VOID;
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t DWORD_PTR;
typedef size_t SIZE_T;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef int32_t HRESULT;
typedef uint32_t COLORREF;
typedef uint16_t ATOM;
typedef DWORD LCID;

typedef char* LPSTR;
typedef const char* LPCSTR;
typedef char* LPTSTR;
typedef const char* LPCTSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPOLESTR;
typedef const wchar_t* LPCOLESTR;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef void* PVOID;
typedef BYTE* LPBYTE;
typedef BYTE* PBYTE;
typedef DWORD* LPDWORD;
typedef WORD* LPWORD;
typedef LONG* LPLONG;
typedef INT* LPINT;
typedef BOOL* LPBOOL;
typedef COLORREF* LPCOLORREF;

typedef void* HANDLE;
typedef void* HGDIOBJ;
typedef void* HGLOBAL;
typedef void* HLOCAL;
#define DTBLKFIX_WIN32_HANDLE(name)                                                                \
  struct name##__ {                                                                                \
    int unused;                                                                                    \
  };                                                                                               \
  typedef struct name##__* name
DTBLKFIX_WIN32_HANDLE(HWND);
DTBLKFIX_WIN32_HANDLE(HDC);
DTBLKFIX_WIN32_HANDLE(HINSTANCE);
DTBLKFIX_WIN32_HANDLE(HBITMAP);
DTBLKFIX_WIN32_HANDLE(HBRUSH);
DTBLKFIX_WIN32_HANDLE(HPEN);
DTBLKFIX_WIN32_HANDLE(HFONT);
DTBLKFIX_WIN32_HANDLE(HRGN);
DTBLKFIX_WIN32_HANDLE(HMENU);
DTBLKFIX_WIN32_HANDLE(HCURSOR);
DTBLKFIX_WIN32_HANDLE(HICON);
DTBLKFIX_WIN32_HANDLE(HRSRC);
DTBLKFIX_WIN32_HANDLE(HDROP);
DTBLKFIX_WIN32_HANDLE(HPALETTE);
DTBLKFIX_WIN32_HANDLE(HKEY);
DTBLKFIX_WIN32_HANDLE(HHOOK);
DTBLKFIX_WIN32_HANDLE(HACCEL);
typedef HINSTANCE HMODULE;

typedef INT_PTR (*FARPROC)();
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void (*TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef UINT_PTR (*LPOFNHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define MAX_PATH 260
#define INFINITE 0xffffffffu

#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define MAKELPARAM(l, h) ((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKEWPARAM(l, h) ((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELRESULT(l, h) ((LRESULT)(DWORD)MAKELONG(l, h))
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#define GET_WHEEL_DELTA_WPARAM(wp) ((short)HIWORD(wp))
#define GET_KEYSTATE_WPARAM(wp) (LOWORD(wp))
#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCE MAKEINTRESOURCEA
#define IS_INTRESOURCE(r) ((((ULONG_PTR)(r)) >> 16) == 0)
#define TEXT(s) s
#define _T(s) s

#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(c) ((BYTE)(c))
#define GetGValue(c) ((BYTE)(((WORD)(c)) >> 8))
#define GetBValue(c) ((BYTE)((c) >> 16))
#define CLR_INVALID 0xffffffffu

#define S_OK ((HRESULT)0)
#define S_FALSE ((HRESULT)1)
#define NOERROR 0
#define E_NOTIMPL ((HRESULT)0x80004001)
#define E_NOINTERFACE ((HRESULT)0x80004002)
#define E_FAIL ((HRESULT)0x80004005)
#define E_UNEXPECTED ((HRESULT)0x8000ffff)
#define E_INVALIDARG ((HRESULT)0x80070057)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define ERROR_SUCCESS 0
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_ALREADY_EXISTS 183

struct POINT {
  LONG x, y;
};
typedef POINT* LPPOINT;
typedef POINT POINTL;
struct POINTS {
  SHORT x, y;
};
struct SIZE {
  LONG cx, cy;
};
typedef SIZE* LPSIZE;
struct RECT {
  LONG left, top, right, bottom;
};
typedef RECT* LPRECT;
typedef const RECT* LPCRECT;

struct tagMSG {
  HWND hwnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD time;
  POINT pt;
};
typedef tagMSG MSG;
typedef MSG* LPMSG;

struct WNDCLASSA {
  UINT style;
  WNDPROC lpfnWndProc;
  int cbClsExtra;
  int cbWndExtra;
  HINSTANCE hInstance;
  HICON hIcon;
  HCURSOR hCursor;
  HBRUSH hbrBackground;
  LPCSTR lpszMenuName;
  LPCSTR lpszClassName;
};
typedef WNDCLASSA WNDCLASS;

struct PAINTSTRUCT {
  HDC hdc;
  BOOL fErase;
  RECT rcPaint;
  BOOL fRestore;
  BOOL fIncUpdate;
  BYTE rgbReserved[32];
};
typedef PAINTSTRUCT* LPPAINTSTRUCT;

struct CREATESTRUCTA {
  LPVOID lpCreateParams;
  HINSTANCE hInstance;
  HMENU hMenu;
  HWND hwndParent;
  int cy, cx, y, x;
  LONG style;
  LPCSTR lpszName;
  LPCSTR lpszClass;
  DWORD dwExStyle;
};
typedef CREATESTRUCTA CREATESTRUCT;
typedef CREATESTRUCTA* LPCREATESTRUCT;

struct WINDOWPOS {
  HWND hwnd, hwndInsertAfter;
  int x, y, cx, cy;
  UINT flags;
};

struct MINMAXINFO {
  POINT ptReserved, ptMaxSize, ptMaxPosition, ptMinTrackSize, ptMaxTrackSize;
};

struct BITMAP {
  LONG bmType, bmWidth, bmHeight, bmWidthBytes;
  WORD bmPlanes, bmBitsPixel;
  LPVOID bmBits;
};

struct RGBQUAD {
  BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved;
};

struct BITMAPINFOHEADER {
  DWORD biSize;
  LONG biWidth, biHeight;
  WORD biPlanes, biBitCount;
  DWORD biCompression, biSizeImage;
  LONG biXPelsPerMeter, biYPelsPerMeter;
  DWORD biClrUsed, biClrImportant;
};
typedef BITMAPINFOHEADER* LPBITMAPINFOHEADER;

struct BITMAPINFO {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD bmiColors[1];
};
typedef BITMAPINFO* LPBITMAPINFO;

struct BITMAPV5HEADER {
  DWORD bV5Size;
  LONG bV5Width, bV5Height;
  WORD bV5Planes, bV5BitCount;
  DWORD bV5Compression, bV5SizeImage;
  LONG bV5XPelsPerMeter, bV5YPelsPerMeter;
  DWORD bV5ClrUsed, bV5ClrImportant;
  DWORD bV5RedMask, bV5GreenMask, bV5BlueMask, bV5AlphaMask;
  DWORD bV5CSType;
  LONG bV5Endpoints[9];
  DWORD bV5GammaRed, bV5GammaGreen, bV5GammaBlue;
  DWORD bV5Intent, bV5ProfileData, bV5ProfileSize, bV5Reserved;
};

struct DIBSECTION {
  BITMAP dsBm;
  BITMAPINFOHEADER dsBmih;
  DWORD dsBitfields[3];
  HANDLE dshSection;
  DWORD dsOffset;
};

#define LF_FACESIZE 32
struct LOGFONTA {
  LONG lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight;
  BYTE lfItalic, lfUnderline, lfStrikeOut, lfCharSet;
  BYTE lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily;
  CHAR lfFaceName[LF_FACESIZE];
};
typedef LOGFONTA LOGFONT;

struct TEXTMETRICA {
  LONG tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading;
  LONG tmAveCharWidth, tmMaxCharWidth, tmWeight, tmOverhang;
  LONG tmDigitizedAspectX, tmDigitizedAspectY;
  BYTE tmFirstChar, tmLastChar, tmDefaultChar, tmBreakChar;
  BYTE tmItalic, tmUnderlined, tmStruckOut, tmPitchAndFamily, tmCharSet;
};
typedef TEXTMETRICA TEXTMETRIC;

struct LOGPEN {
  UINT lopnStyle;
  POINT lopnWidth;
  COLORREF lopnColor;
};

struct LOGBRUSH {
  UINT lbStyle;
  COLORREF lbColor;
  ULONG_PTR lbHatch;
};

struct XFORM {
  FLOAT eM11, eM12, eM21, eM22, eDx, eDy;
};

struct BLENDFUNCTION {
  BYTE BlendOp, BlendFlags, SourceConstantAlpha, AlphaFormat;
};

struct RGNDATAHEADER {
  DWORD dwSize, iType, nCount, nRgnSize;
  RECT rcBound;
};
struct RGNDATA {
  RGNDATAHEADER rdh;
  char Buffer[1];
};
typedef RGNDATA* LPRGNDATA;

struct FILETIME {
  DWORD dwLowDateTime, dwHighDateTime;
};

struct WIN32_FIND_DATAA {
  DWORD dwFileAttributes;
  FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime;
  DWORD nFileSizeHigh, nFileSizeLow, dwReserved0, dwReserved1;
  CHAR cFileName[MAX_PATH];
  CHAR cAlternateFileName[14];
};
typedef WIN32_FIND_DATAA WIN32_FIND_DATA;

struct CRITICAL_SECTION {
  void* impl;
};
typedef CRITICAL_SECTION* LPCRITICAL_SECTION;

struct MEMORY_BASIC_INFORMATION {
  PVOID BaseAddress, AllocationBase;
  DWORD AllocationProtect;
  SIZE_T RegionSize;
  DWORD State, Protect, Type;
};

struct OSVERSIONINFOA {
  DWORD dwOSVersionInfoSize, dwMajorVersion, dwMinorVersion, dwBuildNumber, dwPlatformId;
  CHAR szCSDVersion[128];
};
typedef OSVERSIONINFOA OSVERSIONINFO;
struct OSVERSIONINFOEXA {
  DWORD dwOSVersionInfoSize, dwMajorVersion, dwMinorVersion, dwBuildNumber, dwPlatformId;
  CHAR szCSDVersion[128];
  WORD wServicePackMajor, wServicePackMinor, wSuiteMask;
  BYTE wProductType, wReserved;
};
typedef OSVERSIONINFOEXA OSVERSIONINFOEX;

struct SECURITY_ATTRIBUTES {
  DWORD nLength;
  LPVOID lpSecurityDescriptor;
  BOOL bInheritHandle;
};
typedef SECURITY_ATTRIBUTES* LPSECURITY_ATTRIBUTES;

struct OVERLAPPED {
  int unused;
};
typedef OVERLAPPED* LPOVERLAPPED;

struct TRACKMOUSEEVENT {
  DWORD cbSize, dwFlags;
  HWND hwndTrack;
  DWORD dwHoverTime;
};

struct TPMPARAMS {
  UINT cbSize;
  RECT rcExclude;
};

struct MENUITEMINFOA {
  UINT cbSize, fMask, fType, fState, wID;
  HMENU hSubMenu;
  HBITMAP hbmpChecked, hbmpUnchecked;
  ULONG_PTR dwItemData;
  LPSTR dwTypeData;
  UINT cch;
  HBITMAP hbmpItem;
};
typedef MENUITEMINFOA MENUITEMINFO;

struct MENUINFO {
  DWORD cbSize, fMask, dwStyle;
  UINT cyMax;
  HBRUSH hbrBack;
  DWORD dwContextHelpID;
  ULONG_PTR dwMenuData;
};

struct MEASUREITEMSTRUCT {
  UINT CtlType, CtlID, itemID, itemWidth, itemHeight;
  ULONG_PTR itemData;
};
typedef MEASUREITEMSTRUCT* LPMEASUREITEMSTRUCT;

struct DRAWITEMSTRUCT {
  UINT CtlType, CtlID, itemID, itemAction, itemState;
  HWND hwndItem;
  HDC hDC;
  RECT rcItem;
  ULONG_PTR itemData;
};
typedef DRAWITEMSTRUCT* LPDRAWITEMSTRUCT;

struct OPENFILENAMEA {
  DWORD lStructSize;
  HWND hwndOwner;
  HINSTANCE hInstance;
  LPCSTR lpstrFilter;
  LPSTR lpstrCustomFilter;
  DWORD nMaxCustFilter, nFilterIndex;
  LPSTR lpstrFile;
  DWORD nMaxFile;
  LPSTR lpstrFileTitle;
  DWORD nMaxFileTitle;
  LPCSTR lpstrInitialDir, lpstrTitle;
  DWORD Flags;
  WORD nFileOffset, nFileExtension;
  LPCSTR lpstrDefExt;
  LPARAM lCustData;
  LPOFNHOOKPROC lpfnHook;
  LPCSTR lpTemplateName;
};
typedef OPENFILENAMEA OPENFILENAME;

struct GUID {
  uint32_t Data1;
  uint16_t Data2, Data3;
  uint8_t Data4[8];
};
typedef GUID IID;
typedef GUID CLSID;
typedef const GUID& REFIID;
typedef const GUID& REFCLSID;
typedef const GUID& REFGUID;
inline bool operator==(const GUID& a, const GUID& b)
{
  return memcmp(&a, &b, sizeof(GUID)) == 0;
}
inline bool operator!=(const GUID& a, const GUID& b)
{
  return !(a == b);
}
#define IsEqualIID(a, b) ((a) == (b))
#define IsEqualGUID(a, b) ((a) == (b))

#define STDMETHODCALLTYPE
#define STDMETHOD(name) virtual HRESULT name
#define STDMETHOD_(type, name) virtual type name
#define STDMETHODIMP HRESULT
#define STDMETHODIMP_(type) type
#define PURE = 0
#define THIS_
#define THIS void

struct IUnknown {
  virtual HRESULT QueryInterface(REFIID riid, void** object) = 0;
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
};
typedef IUnknown* LPUNKNOWN;

typedef WORD CLIPFORMAT;
struct DVTARGETDEVICE;
struct FORMATETC {
  CLIPFORMAT cfFormat;
  DVTARGETDEVICE* ptd;
  DWORD dwAspect;
  LONG lindex;
  DWORD tymed;
};
struct STGMEDIUM {
  DWORD tymed;
  union {
    HBITMAP hBitmap;
    HGLOBAL hGlobal;
    LPOLESTR lpszFileName;
  };
  IUnknown* pUnkForRelease;
};
struct IDataObject : public IUnknown {
  virtual HRESULT GetData(FORMATETC* format, STGMEDIUM* medium) = 0;
  virtual HRESULT QueryGetData(FORMATETC* format) = 0;
};
typedef IDataObject* LPDATAOBJECT;
struct IDropTarget : public IUnknown {
  virtual HRESULT DragEnter(IDataObject* data, DWORD key_state, POINTL pt, DWORD* effect) = 0;
  virtual HRESULT DragOver(DWORD key_state, POINTL pt, DWORD* effect) = 0;
  virtual HRESULT DragLeave() = 0;
  virtual HRESULT Drop(IDataObject* data, DWORD key_state, POINTL pt, DWORD* effect) = 0;
};
typedef IDropTarget* LPDROPTARGET;
struct IPersistFile : public IUnknown {
  virtual HRESULT Load(LPCOLESTR file_name, DWORD mode) = 0;
};
struct IShellLinkA : public IUnknown {
  virtual HRESULT GetPath(LPSTR file, int cch, WIN32_FIND_DATAA* fd, DWORD flags) = 0;
  virtual HRESULT Resolve(HWND hwnd, DWORD flags) = 0;
};
typedef IShellLinkA IShellLink;

extern const IID IID_IUnknown;
extern const IID IID_IDropTarget;
extern const IID IID_IPersistFile;
extern const IID IID_IShellLinkA;
extern const CLSID CLSID_ShellLink;
#define IID_IShellLink IID_IShellLinkA

#define DVASPECT_CONTENT 1
#define TYMED_HGLOBAL 1
#define CF_TEXT 1
#define CF_HDROP 15
#define DROPEFFECT_NONE 0
#define DROPEFFECT_COPY 1
#define DROPEFFECT_MOVE 2
#define DROPEFFECT_LINK 4
#define CLSCTX_INPROC_SERVER 1
#define STGM_READ 0
#define SLGP_SHORTPATH 1
#define SLR_NO_UI 1
#define SLR_ANY_MATCH 2
#define CP_ACP 0
#define CP_UTF8 65001
#define MB_PRECOMPOSED 1

#define WM_NULL 0x0000
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
#define WM_ACTIVATE 0x0006
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_ENABLE 0x000a
#define WM_SETTEXT 0x000c
#define WM_GETTEXT 0x000d
#define WM_GETTEXTLENGTH 0x000e
#define WM_PAINT 0x000f
#define WM_CLOSE 0x0010
#define WM_QUIT 0x0012
#define WM_ERASEBKGND 0x0014
#define WM_SHOWWINDOW 0x0018
#define WM_SETCURSOR 0x0020
#define WM_MOUSEACTIVATE 0x0021
#define WM_GETMINMAXINFO 0x0024
#define WM_DRAWITEM 0x002b
#define WM_MEASUREITEM 0x002c
#define WM_SETFONT 0x0030
#define WM_GETFONT 0x0031
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED 0x0047
#define WM_NOTIFY 0x004e
#define WM_CONTEXTMENU 0x007b
#define WM_NCCREATE 0x0081
#define WM_NCDESTROY 0x0082
#define WM_NCHITTEST 0x0084
#define WM_NCPAINT 0x0085
#define WM_GETDLGCODE 0x0087
#define WM_KEYFIRST 0x0100
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define WM_SYSCHAR 0x0106
#define WM_KEYLAST 0x0108
#define WM_INITDIALOG 0x0110
#define WM_COMMAND 0x0111
#define WM_SYSCOMMAND 0x0112
#define WM_TIMER 0x0113
#define WM_HSCROLL 0x0114
#define WM_VSCROLL 0x0115
#define WM_INITMENU 0x0116
#define WM_INITMENUPOPUP 0x0117
#define WM_MENUSELECT 0x011f
#define WM_CTLCOLOREDIT 0x0133
#define WM_CTLCOLORSTATIC 0x0138
#define WM_MOUSEFIRST 0x0200
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_RBUTTONDBLCLK 0x0206
#define WM_MBUTTONDOWN 0x0207
#define WM_MBUTTONUP 0x0208
#define WM_MBUTTONDBLCLK 0x0209
#define WM_MOUSEWHEEL 0x020a
#define WM_XBUTTONDOWN 0x020b
#define WM_XBUTTONUP 0x020c
#define WM_XBUTTONDBLCLK 0x020d
#define WM_MOUSEHWHEEL 0x020e
#define WM_MOUSELAST 0x020e
#define WM_CAPTURECHANGED 0x0215
#define WM_MOUSEHOVER 0x02a1
#define WM_MOUSELEAVE 0x02a3
#define WM_DROPFILES 0x0233
#define WM_CUT 0x0300
#define WM_COPY 0x0301
#define WM_PASTE 0x0302
#define WM_PRINT 0x0317
#define WM_PRINTCLIENT 0x0318
#define WM_USER 0x0400
#define WM_APP 0x8000
#define WHEEL_DELTA 120

#define EM_GETSEL 0x00b0
#define EM_SETSEL 0x00b1
#define EM_LIMITTEXT 0x00c5
#define EM_SETLIMITTEXT EM_LIMITTEXT
#define EM_SETMARGINS 0x00d3
#define EC_LEFTMARGIN 0x0001
#define EC_RIGHTMARGIN 0x0002
#define EN_SETFOCUS 0x0100
#define EN_KILLFOCUS 0x0200
#define EN_CHANGE 0x0300
#define EN_UPDATE 0x0400

#define MK_LBUTTON 0x0001
#define MK_RBUTTON 0x0002
#define MK_SHIFT 0x0004
#define MK_CONTROL 0x0008
#define MK_MBUTTON 0x0010
#define MK_XBUTTON1 0x0020
#define MK_XBUTTON2 0x0040

#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_CANCEL 0x03
#define VK_MBUTTON 0x04
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0c
#define VK_RETURN 0x0d
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1b
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SELECT 0x29
#define VK_PRINT 0x2a
#define VK_EXECUTE 0x2b
#define VK_SNAPSHOT 0x2c
#define VK_INSERT 0x2d
#define VK_DELETE 0x2e
#define VK_HELP 0x2f
#define VK_LWIN 0x5b
#define VK_RWIN 0x5c
#define VK_APPS 0x5d
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6a
#define VK_ADD 0x6b
#define VK_SEPARATOR 0x6c
#define VK_SUBTRACT 0x6d
#define VK_DECIMAL 0x6e
#define VK_DIVIDE 0x6f
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7a
#define VK_F12 0x7b
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_LSHIFT 0xa0
#define VK_RSHIFT 0xa1
#define VK_LCONTROL 0xa2
#define VK_RCONTROL 0xa3
#define VK_LMENU 0xa4
#define VK_RMENU 0xa5

#define WS_OVERLAPPED 0x00000000u
#define WS_POPUP 0x80000000u
#define WS_CHILD 0x40000000u
#define WS_MINIMIZE 0x20000000u
#define WS_VISIBLE 0x10000000u
#define WS_DISABLED 0x08000000u
#define WS_CLIPSIBLINGS 0x04000000u
#define WS_CLIPCHILDREN 0x02000000u
#define WS_MAXIMIZE 0x01000000u
#define WS_CAPTION 0x00c00000u
#define WS_BORDER 0x00800000u
#define WS_DLGFRAME 0x00400000u
#define WS_VSCROLL 0x00200000u
#define WS_HSCROLL 0x00100000u
#define WS_SYSMENU 0x00080000u
#define WS_THICKFRAME 0x00040000u
#define WS_GROUP 0x00020000u
#define WS_TABSTOP 0x00010000u
#define WS_MINIMIZEBOX 0x00020000u
#define WS_MAXIMIZEBOX 0x00010000u
#define WS_OVERLAPPEDWINDOW                                                                        \
  (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_EX_DLGMODALFRAME 0x00000001u
#define WS_EX_TOPMOST 0x00000008u
#define WS_EX_ACCEPTFILES 0x00000010u
#define WS_EX_TRANSPARENT 0x00000020u
#define WS_EX_TOOLWINDOW 0x00000080u
#define WS_EX_CLIENTEDGE 0x00000200u
#define WS_EX_LAYERED 0x00080000u
#define WS_EX_NOACTIVATE 0x08000000u

#define ES_LEFT 0x0000
#define ES_CENTER 0x0001
#define ES_RIGHT 0x0002
#define ES_MULTILINE 0x0004
#define ES_UPPERCASE 0x0008
#define ES_LOWERCASE 0x0010
#define ES_PASSWORD 0x0020
#define ES_AUTOVSCROLL 0x0040
#define ES_AUTOHSCROLL 0x0080
#define ES_NOHIDESEL 0x0100
#define ES_READONLY 0x0800
#define ES_WANTRETURN 0x1000
#define ES_NUMBER 0x2000

#define CS_VREDRAW 0x0001
#define CS_HREDRAW 0x0002
#define CS_DBLCLKS 0x0008
#define CS_OWNDC 0x0020
#define CS_GLOBALCLASS 0x4000

#define CW_USEDEFAULT ((int)0x80000000)
#define HWND_DESKTOP ((HWND)0)
#define HWND_TOP ((HWND)0)
#define HWND_BOTTOM ((HWND)1)
#define HWND_TOPMOST ((HWND)-1)
#define HWND_NOTOPMOST ((HWND)-2)

#define GWL_WNDPROC (-4)
#define GWL_HINSTANCE (-6)
#define GWL_HWNDPARENT (-8)
#define GWL_ID (-12)
#define GWL_STYLE (-16)
#define GWL_EXSTYLE (-20)
#define GWL_USERDATA (-21)
#define GWLP_WNDPROC (-4)
#define GWLP_HINSTANCE (-6)
#define GWLP_HWNDPARENT (-8)
#define GWLP_ID (-12)
#define GWLP_USERDATA (-21)

#define GW_HWNDFIRST 0
#define GW_HWNDLAST 1
#define GW_HWNDNEXT 2
#define GW_HWNDPREV 3
#define GW_OWNER 4
#define GW_CHILD 5

#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_NORMAL 1
#define SW_SHOWNOACTIVATE 4
#define SW_SHOW 5
#define SW_SHOWNA 8

#define SWP_NOSIZE 0x0001
#define SWP_NOMOVE 0x0002
#define SWP_NOZORDER 0x0004
#define SWP_NOREDRAW 0x0008
#define SWP_NOACTIVATE 0x0010
#define SWP_FRAMECHANGED 0x0020
#define SWP_SHOWWINDOW 0x0040
#define SWP_HIDEWINDOW 0x0080
#define SWP_NOCOPYBITS 0x0100
#define SWP_NOOWNERZORDER 0x0200
#define SWP_NOSENDCHANGING 0x0400

#define PM_NOREMOVE 0x0000
#define PM_REMOVE 0x0001
#define PM_NOYIELD 0x0002

#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define SM_CYCAPTION 4
#define SM_CXBORDER 5
#define SM_CYBORDER 6
#define SM_CXDLGFRAME 7
#define SM_CYDLGFRAME 8
#define SM_CXFIXEDFRAME 7
#define SM_CYFIXEDFRAME 8
#define SM_SWAPBUTTON 23
#define SM_CXDOUBLECLK 36
#define SM_CYDOUBLECLK 37
#define SM_CXDRAG 68
#define SM_CYDRAG 69

#define COLOR_SCROLLBAR 0
#define COLOR_BACKGROUND 1
#define COLOR_MENU 4
#define COLOR_WINDOW 5
#define COLOR_WINDOWFRAME 6
#define COLOR_MENUTEXT 7
#define COLOR_WINDOWTEXT 8
#define COLOR_HIGHLIGHT 13
#define COLOR_HIGHLIGHTTEXT 14
#define COLOR_BTNFACE 15
#define COLOR_BTNSHADOW 16
#define COLOR_GRAYTEXT 17
#define COLOR_BTNTEXT 18
#define COLOR_3DFACE 15

#define IDC_ARROW ((LPCSTR)(ULONG_PTR)32512)
#define IDC_IBEAM ((LPCSTR)(ULONG_PTR)32513)
#define IDC_WAIT ((LPCSTR)(ULONG_PTR)32514)
#define IDC_CROSS ((LPCSTR)(ULONG_PTR)32515)
#define IDC_UPARROW ((LPCSTR)(ULONG_PTR)32516)
#define IDC_SIZENWSE ((LPCSTR)(ULONG_PTR)32642)
#define IDC_SIZENESW ((LPCSTR)(ULONG_PTR)32643)
#define IDC_SIZEWE ((LPCSTR)(ULONG_PTR)32644)
#define IDC_SIZENS ((LPCSTR)(ULONG_PTR)32645)
#define IDC_SIZEALL ((LPCSTR)(ULONG_PTR)32646)
#define IDC_NO ((LPCSTR)(ULONG_PTR)32648)
#define IDC_HAND ((LPCSTR)(ULONG_PTR)32649)

#define MB_OK 0x0000
#define MB_OKCANCEL 0x0001
#define MB_YESNO 0x0004
#define MB_ICONERROR 0x0010
#define MB_ICONWARNING 0x0030
#define MB_ICONINFORMATION 0x0040
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7

#define MF_BYCOMMAND 0x0000
#define MF_BYPOSITION 0x0400
#define MF_ENABLED 0x0000
#define MF_GRAYED 0x0001
#define MF_DISABLED 0x0002
#define MF_BITMAP 0x0004
#define MF_UNCHECKED 0x0000
#define MF_CHECKED 0x0008
#define MF_STRING 0x0000
#define MF_POPUP 0x0010
#define MF_MENUBARBREAK 0x0020
#define MF_MENUBREAK 0x0040
#define MF_HILITE 0x0080
#define MF_OWNERDRAW 0x0100
#define MF_SEPARATOR 0x0800
#define MIIM_STATE 0x0001
#define MIIM_ID 0x0002
#define MIIM_SUBMENU 0x0004
#define MIIM_TYPE 0x0010
#define MIIM_DATA 0x0020
#define MIM_BACKGROUND 0x0002
#define MIM_STYLE 0x0010
#define MIM_APPLYTOSUBMENUS 0x80000000u
#define MFT_STRING 0x0000
#define MFT_SEPARATOR 0x0800
#define MFS_CHECKED 0x0008
#define MFS_GRAYED 0x0003
#define TPM_LEFTBUTTON 0x0000
#define TPM_RIGHTBUTTON 0x0002
#define TPM_LEFTALIGN 0x0000
#define TPM_CENTERALIGN 0x0004
#define TPM_RIGHTALIGN 0x0008
#define TPM_TOPALIGN 0x0000
#define TPM_VCENTERALIGN 0x0010
#define TPM_BOTTOMALIGN 0x0020
#define TPM_NONOTIFY 0x0080
#define TPM_RETURNCMD 0x0100
#define ODT_MENU 1
#define ODS_SELECTED 0x0001
#define ODS_GRAYED 0x0002
#define ODS_DISABLED 0x0004
#define ODS_CHECKED 0x0008

#define DLGC_WANTARROWS 0x0001
#define DLGC_WANTTAB 0x0002
#define DLGC_WANTALLKEYS 0x0004
#define DLGC_WANTCHARS 0x0080

#define TME_HOVER 0x0001
#define TME_LEAVE 0x0002
#define TME_CANCEL 0x80000000u

#define ULW_COLORKEY 1
#define ULW_ALPHA 2
#define ULW_OPAQUE 4
#define LWA_COLORKEY 1
#define LWA_ALPHA 2

#define AC_SRC_OVER 0
#define AC_SRC_ALPHA 1

#define SRCCOPY 0x00cc0020u
#define SRCPAINT 0x00ee0086u
#define SRCAND 0x008800c6u
#define SRCINVERT 0x00660046u
#define SRCERASE 0x00440328u
#define NOTSRCCOPY 0x00330008u
#define NOTSRCERASE 0x001100a6u
#define MERGECOPY 0x00c000cau
#define MERGEPAINT 0x00bb0226u
#define PATCOPY 0x00f00021u
#define PATPAINT 0x00fb0a09u
#define PATINVERT 0x005a0049u
#define DSTINVERT 0x00550009u
#define BLACKNESS 0x00000042u
#define WHITENESS 0x00ff0062u

#define R2_BLACK 1
#define R2_NOTMERGEPEN 2
#define R2_MASKNOTPEN 3
#define R2_NOTCOPYPEN 4
#define R2_MASKPENNOT 5
#define R2_NOT 6
#define R2_XORPEN 7
#define R2_NOTMASKPEN 8
#define R2_MASKPEN 9
#define R2_NOTXORPEN 10
#define R2_NOP 11
#define R2_MERGENOTPEN 12
#define R2_COPYPEN 13
#define R2_MERGEPENNOT 14
#define R2_MERGEPEN 15
#define R2_WHITE 16

#define TRANSPARENT 1
#define OPAQUE 2
#define GM_COMPATIBLE 1
#define GM_ADVANCED 2
#define MWT_IDENTITY 1
#define MWT_LEFTMULTIPLY 2
#define MWT_RIGHTMULTIPLY 3
#define MM_TEXT 1

#define BLACKONWHITE 1
#define WHITEONBLACK 2
#define COLORONCOLOR 3
#define HALFTONE 4
#define STRETCH_DELETESCANS COLORONCOLOR
#define STRETCH_HALFTONE HALFTONE

#define PS_SOLID 0
#define PS_DASH 1
#define PS_DOT 2
#define PS_DASHDOT 3
#define PS_DASHDOTDOT 4
#define PS_NULL 5
#define PS_INSIDEFRAME 6
#define PS_GEOMETRIC 0x00010000
#define PS_ENDCAP_ROUND 0x00000000
#define PS_ENDCAP_SQUARE 0x00000100
#define PS_ENDCAP_FLAT 0x00000200
#define PS_JOIN_ROUND 0x00000000
#define PS_JOIN_BEVEL 0x00001000
#define PS_JOIN_MITER 0x00002000
#define BS_SOLID 0
#define BS_NULL 1
#define BS_HOLLOW BS_NULL

#define WHITE_BRUSH 0
#define LTGRAY_BRUSH 1
#define GRAY_BRUSH 2
#define DKGRAY_BRUSH 3
#define BLACK_BRUSH 4
#define NULL_BRUSH 5
#define HOLLOW_BRUSH NULL_BRUSH
#define WHITE_PEN 6
#define BLACK_PEN 7
#define NULL_PEN 8
#define OEM_FIXED_FONT 10
#define ANSI_FIXED_FONT 11
#define ANSI_VAR_FONT 12
#define SYSTEM_FONT 13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE 15
#define SYSTEM_FIXED_FONT 16
#define DEFAULT_GUI_FONT 17

#define OBJ_PEN 1
#define OBJ_BRUSH 2
#define OBJ_DC 3
#define OBJ_FONT 6
#define OBJ_BITMAP 7
#define OBJ_REGION 8
#define OBJ_MEMDC 10

#define ALTERNATE 1
#define WINDING 2
#define FLOODFILLBORDER 0
#define FLOODFILLSURFACE 1

#define RGN_AND 1
#define RGN_OR 2
#define RGN_XOR 3
#define RGN_DIFF 4
#define RGN_COPY 5
#define ERROR 0
#define NULLREGION 1
#define SIMPLEREGION 2
#define COMPLEXREGION 3
#define RDH_RECTANGLES 1

#define BI_RGB 0
#define BI_BITFIELDS 3
#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1
#define IMAGE_BITMAP 0
#define IMAGE_ICON 1
#define IMAGE_CURSOR 2
#define LR_DEFAULTCOLOR 0x0000
#define LR_LOADFROMFILE 0x0010
#define LR_CREATEDIBSECTION 0x2000

#define FW_DONTCARE 0
#define FW_THIN 100
#define FW_LIGHT 300
#define FW_NORMAL 400
#define FW_REGULAR 400
#define FW_MEDIUM 500
#define FW_SEMIBOLD 600
#define FW_BOLD 700
#define FW_HEAVY 900
#define ANSI_CHARSET 0
#define DEFAULT_CHARSET 1
#define SYMBOL_CHARSET 2
#define OUT_DEFAULT_PRECIS 0
#define OUT_STRING_PRECIS 1
#define OUT_TT_PRECIS 4
#define CLIP_DEFAULT_PRECIS 0
#define CLIP_STROKE_PRECIS 2
#define DEFAULT_QUALITY 0
#define DRAFT_QUALITY 1
#define PROOF_QUALITY 2
#define NONANTIALIASED_QUALITY 3
#define ANTIALIASED_QUALITY 4
#define CLEARTYPE_QUALITY 5
#define DEFAULT_PITCH 0
#define FIXED_PITCH 1
#define VARIABLE_PITCH 2
#define FF_DONTCARE (0 << 4)
#define FF_ROMAN (1 << 4)
#define FF_SWISS (2 << 4)
#define FF_MODERN (3 << 4)
#define FF_SCRIPT (4 << 4)
#define FF_DECORATIVE (5 << 4)

#define DT_TOP 0x0000
#define DT_LEFT 0x0000
#define DT_CENTER 0x0001
#define DT_RIGHT 0x0002
#define DT_VCENTER 0x0004
#define DT_BOTTOM 0x0008
#define DT_WORDBREAK 0x0010
#define DT_SINGLELINE 0x0020
#define DT_EXPANDTABS 0x0040
#define DT_NOCLIP 0x0100
#define DT_CALCRECT 0x0400
#define DT_NOPREFIX 0x0800
#define DT_END_ELLIPSIS 0x8000
#define TA_LEFT 0
#define TA_RIGHT 2
#define TA_CENTER 6
#define TA_TOP 0
#define TA_BOTTOM 8
#define TA_BASELINE 24
#define ETO_OPAQUE 0x0002
#define ETO_CLIPPED 0x0004

#define GENERIC_READ 0x80000000u
#define GENERIC_WRITE 0x40000000u
#define FILE_SHARE_READ 0x00000001
#define FILE_SHARE_WRITE 0x00000002
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_HIDDEN 0x00000002
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define RT_BITMAP MAKEINTRESOURCEA(2)
#define RT_RCDATA MAKEINTRESOURCEA(10)
#define CSIDL_PERSONAL 0x0005
#define CSIDL_APPDATA 0x001a
#define CSIDL_LOCAL_APPDATA 0x001c
#define VER_PLATFORM_WIN32_WINDOWS 1
#define VER_PLATFORM_WIN32_NT 2
#define GMEM_FIXED 0x0000
#define GMEM_MOVEABLE 0x0002
#define GHND 0x0042

#define OFN_READONLY 0x00000001
#define OFN_OVERWRITEPROMPT 0x00000002
#define OFN_HIDEREADONLY 0x00000004
#define OFN_NOCHANGEDIR 0x00000008
#define OFN_ENABLEHOOK 0x00000020
#define OFN_ALLOWMULTISELECT 0x00000200
#define OFN_PATHMUSTEXIST 0x00000800
#define OFN_FILEMUSTEXIST 0x00001000
#define OFN_EXPLORER 0x00080000
#define OFN_ENABLESIZING 0x00800000

#ifndef _TRUNCATE
#  define _TRUNCATE ((size_t)-1)
#endif

typedef BOOL (*PFNALPHABLEND)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
typedef BOOL (*PFNTRANSPARENTBLT)(HDC, int, int, int, int, HDC, int, int, int, int, UINT);

void InitializeCriticalSection(LPCRITICAL_SECTION cs);
void DeleteCriticalSection(LPCRITICAL_SECTION cs);
void EnterCriticalSection(LPCRITICAL_SECTION cs);
void LeaveCriticalSection(LPCRITICAL_SECTION cs);
LONG InterlockedIncrement(LONG volatile* value);
LONG InterlockedDecrement(LONG volatile* value);
LONG InterlockedExchange(LONG volatile* target, LONG value);
DWORD GetTickCount();
void Sleep(DWORD ms);
DWORD GetLastError();
void SetLastError(DWORD error);
DWORD GetCurrentProcessId();
DWORD GetCurrentThreadId();
BOOL GetVersionExA(OSVERSIONINFOA* info);
void OutputDebugStringA(LPCSTR text);
int MulDiv(int number, int numerator, int denominator);
int MultiByteToWideChar(UINT cp, DWORD flags, LPCSTR src, int src_len, LPWSTR dst, int dst_len);
int WideCharToMultiByte(UINT cp, DWORD flags, LPCWSTR src, int src_len, LPSTR dst, int dst_len,
                        LPCSTR default_char, LPBOOL used_default);
int lstrlenA(LPCSTR s);
LPSTR lstrcpyA(LPSTR dst, LPCSTR src);
LPSTR lstrcpynA(LPSTR dst, LPCSTR src, int max_len);
LPSTR lstrcatA(LPSTR dst, LPCSTR src);
int lstrcmpA(LPCSTR a, LPCSTR b);
int lstrcmpiA(LPCSTR a, LPCSTR b);
int wsprintfA(LPSTR dst, LPCSTR format, ...);

HMODULE LoadLibraryA(LPCSTR name);
BOOL FreeLibrary(HMODULE module);
FARPROC GetProcAddress(HMODULE module, LPCSTR name);
HMODULE GetModuleHandleA(LPCSTR name);
DWORD GetModuleFileNameA(HMODULE module, LPSTR file, DWORD size);
SIZE_T VirtualQuery(LPCVOID address, MEMORY_BASIC_INFORMATION* info, SIZE_T size);
HRSRC FindResourceA(HMODULE module, LPCSTR name, LPCSTR type);
HGLOBAL LoadResource(HMODULE module, HRSRC resource);
LPVOID LockResource(HGLOBAL data);
DWORD SizeofResource(HMODULE module, HRSRC resource);
HGLOBAL GlobalAlloc(UINT flags, SIZE_T bytes);
HGLOBAL GlobalFree(HGLOBAL mem);
LPVOID GlobalLock(HGLOBAL mem);
BOOL GlobalUnlock(HGLOBAL mem);
SIZE_T GlobalSize(HGLOBAL mem);

DWORD GetEnvironmentVariableA(LPCSTR name, LPSTR buffer, DWORD size);
BOOL SetEnvironmentVariableA(LPCSTR name, LPCSTR value);
DWORD GetFileAttributesA(LPCSTR file);
BOOL CreateDirectoryA(LPCSTR dir, LPSECURITY_ATTRIBUTES security);
BOOL RemoveDirectoryA(LPCSTR dir);
BOOL DeleteFileA(LPCSTR file);
BOOL MoveFileA(LPCSTR from, LPCSTR to);
HANDLE CreateFileA(LPCSTR file, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security,
                   DWORD creation, DWORD flags, HANDLE template_file);
BOOL WriteFile(HANDLE file, LPCVOID data, DWORD size, LPDWORD written, LPOVERLAPPED overlapped);
BOOL ReadFile(HANDLE file, LPVOID data, DWORD size, LPDWORD read, LPOVERLAPPED overlapped);
DWORD GetFileSize(HANDLE file, LPDWORD high);
BOOL CloseHandle(HANDLE handle);
HANDLE FindFirstFileA(LPCSTR pattern, WIN32_FIND_DATAA* data);
BOOL FindNextFileA(HANDLE find, WIN32_FIND_DATAA* data);
BOOL FindClose(HANDLE find);
DWORD GetCurrentDirectoryA(DWORD size, LPSTR buffer);
BOOL SetCurrentDirectoryA(LPCSTR dir);
DWORD GetTempPathA(DWORD size, LPSTR buffer);
HRESULT SHGetFolderPathA(HWND owner, int folder, HANDLE token, DWORD flags, LPSTR path);
UINT DragQueryFileA(HDROP drop, UINT index, LPSTR file, UINT size);
void DragFinish(HDROP drop);
void DragAcceptFiles(HWND hwnd, BOOL accept);
HINSTANCE ShellExecuteA(HWND hwnd, LPCSTR operation, LPCSTR file, LPCSTR parameters, LPCSTR dir,
                        INT show);

HRESULT OleInitialize(LPVOID reserved);
void OleUninitialize();
HRESULT CoInitialize(LPVOID reserved);
void CoUninitialize();
HRESULT CoCreateInstance(REFCLSID clsid, LPUNKNOWN outer, DWORD context, REFIID iid, LPVOID* out);
HRESULT RegisterDragDrop(HWND hwnd, LPDROPTARGET target);
HRESULT RevokeDragDrop(HWND hwnd);
void ReleaseStgMedium(STGMEDIUM* medium);

ATOM RegisterClassA(const WNDCLASSA* wc);
BOOL UnregisterClassA(LPCSTR class_name, HINSTANCE instance);
BOOL GetClassInfoA(HINSTANCE instance, LPCSTR class_name, WNDCLASSA* wc);
int GetClassNameA(HWND hwnd, LPSTR class_name, int max_count);
HWND CreateWindowExA(DWORD ex_style, LPCSTR class_name, LPCSTR window_name, DWORD style, int x,
                     int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance,
                     LPVOID param);
BOOL DestroyWindow(HWND hwnd);
BOOL IsWindow(HWND hwnd);
BOOL IsWindowVisible(HWND hwnd);
BOOL IsWindowEnabled(HWND hwnd);
BOOL EnableWindow(HWND hwnd, BOOL enable);
BOOL ShowWindow(HWND hwnd, int show);
BOOL UpdateWindow(HWND hwnd);
BOOL MoveWindow(HWND hwnd, int x, int y, int width, int height, BOOL repaint);
BOOL SetWindowPos(HWND hwnd, HWND insert_after, int x, int y, int cx, int cy, UINT flags);
BOOL GetWindowRect(HWND hwnd, LPRECT rect);
BOOL GetClientRect(HWND hwnd, LPRECT rect);
BOOL AdjustWindowRect(LPRECT rect, DWORD style, BOOL menu);
BOOL AdjustWindowRectEx(LPRECT rect, DWORD style, BOOL menu, DWORD ex_style);
BOOL ClientToScreen(HWND hwnd, LPPOINT point);
BOOL ScreenToClient(HWND hwnd, LPPOINT point);
int MapWindowPoints(HWND from, HWND to, LPPOINT points, UINT count);
HWND GetParent(HWND hwnd);
HWND SetParent(HWND child, HWND parent);
HWND GetWindow(HWND hwnd, UINT cmd);
HWND GetDesktopWindow();
HWND FindWindowExA(HWND parent, HWND child_after, LPCSTR class_name, LPCSTR window_name);
HWND GetFocus();
HWND SetFocus(HWND hwnd);
HWND GetCapture();
HWND SetCapture(HWND hwnd);
BOOL ReleaseCapture();
HWND GetActiveWindow();
HWND GetForegroundWindow();
BOOL SetForegroundWindow(HWND hwnd);
HWND WindowFromPoint(POINT point);
HWND ChildWindowFromPoint(HWND parent, POINT point);
LONG GetWindowLongA(HWND hwnd, int index);
LONG SetWindowLongA(HWND hwnd, int index, LONG value);
LONG_PTR GetWindowLongPtrA(HWND hwnd, int index);
LONG_PTR SetWindowLongPtrA(HWND hwnd, int index, LONG_PTR value);
BOOL SetPropA(HWND hwnd, LPCSTR name, HANDLE data);
HANDLE GetPropA(HWND hwnd, LPCSTR name);
HANDLE RemovePropA(HWND hwnd, LPCSTR name);
BOOL SetWindowTextA(HWND hwnd, LPCSTR text);
int GetWindowTextA(HWND hwnd, LPSTR text, int max_count);
int GetWindowTextLengthA(HWND hwnd);
int SetWindowRgn(HWND hwnd, HRGN region, BOOL redraw);
int GetWindowRgn(HWND hwnd, HRGN region);
BOOL SetLayeredWindowAttributes(HWND hwnd, COLORREF key, BYTE alpha, DWORD flags);
BOOL UpdateLayeredWindow(HWND hwnd, HDC dst, POINT* dst_pos, SIZE* size, HDC src, POINT* src_pos,
                         COLORREF key, BLENDFUNCTION* blend, DWORD flags);
BOOL TrackMouseEvent(TRACKMOUSEEVENT* event);

LRESULT DefWindowProcA(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CallWindowProcA(WNDPROC proc, HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT SendMessageA(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL PostMessageA(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL PeekMessageA(LPMSG msg, HWND hwnd, UINT first, UINT last, UINT remove);
BOOL GetMessageA(LPMSG msg, HWND hwnd, UINT first, UINT last);
BOOL TranslateMessage(const MSG* msg);
LRESULT DispatchMessageA(const MSG* msg);
void PostQuitMessage(int exit_code);
LONG GetMessageTime();
DWORD GetMessagePos();
UINT_PTR SetTimer(HWND hwnd, UINT_PTR id, UINT elapse, TIMERPROC proc);
BOOL KillTimer(HWND hwnd, UINT_PTR id);
int MessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type);
BOOL MessageBeep(UINT type);

BOOL GetCursorPos(LPPOINT point);
BOOL SetCursorPos(int x, int y);
HCURSOR LoadCursorA(HINSTANCE instance, LPCSTR name);
HCURSOR SetCursor(HCURSOR cursor);
HCURSOR GetCursor();
int ShowCursor(BOOL show);
SHORT GetKeyState(int key);
SHORT GetAsyncKeyState(int key);
UINT GetDoubleClickTime();
int GetSystemMetrics(int index);
DWORD GetSysColor(int index);
HBRUSH GetSysColorBrush(int index);
BOOL OpenClipboard(HWND owner);
BOOL CloseClipboard();
BOOL EmptyClipboard();
HANDLE GetClipboardData(UINT format);
HANDLE SetClipboardData(UINT format, HANDLE data);
BOOL IsClipboardFormatAvailable(UINT format);

HMENU CreatePopupMenu();
HMENU CreateMenu();
BOOL DestroyMenu(HMENU menu);
BOOL AppendMenuA(HMENU menu, UINT flags, UINT_PTR id, LPCSTR item);
BOOL InsertMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR id, LPCSTR item);
BOOL TrackPopupMenu(HMENU menu, UINT flags, int x, int y, int reserved, HWND hwnd,
                    const RECT* rect);
BOOL TrackPopupMenuEx(HMENU menu, UINT flags, int x, int y, HWND hwnd, TPMPARAMS* params);
DWORD CheckMenuItem(HMENU menu, UINT item, UINT check);
BOOL EnableMenuItem(HMENU menu, UINT item, UINT enable);
int GetMenuItemCount(HMENU menu);
UINT GetMenuItemID(HMENU menu, int position);
HMENU GetSubMenu(HMENU menu, int position);
BOOL SetMenuInfo(HMENU menu, const MENUINFO* info);
BOOL SetMenuItemInfoA(HMENU menu, UINT item, BOOL by_position, const MENUITEMINFOA* info);
BOOL GetMenuItemInfoA(HMENU menu, UINT item, BOOL by_position, MENUITEMINFOA* info);
BOOL GetOpenFileNameA(OPENFILENAMEA* ofn);
BOOL GetSaveFileNameA(OPENFILENAMEA* ofn);
DWORD CommDlgExtendedError();

HDC GetDC(HWND hwnd);
HDC GetWindowDC(HWND hwnd);
int ReleaseDC(HWND hwnd, HDC dc);
HDC BeginPaint(HWND hwnd, LPPAINTSTRUCT ps);
BOOL EndPaint(HWND hwnd, const PAINTSTRUCT* ps);
BOOL InvalidateRect(HWND hwnd, const RECT* rect, BOOL erase);
BOOL ValidateRect(HWND hwnd, const RECT* rect);
BOOL InvalidateRgn(HWND hwnd, HRGN region, BOOL erase);
BOOL GetUpdateRect(HWND hwnd, LPRECT rect, BOOL erase);
int GetUpdateRgn(HWND hwnd, HRGN region, BOOL erase);
BOOL RedrawWindow(HWND hwnd, const RECT* rect, HRGN region, UINT flags);
BOOL ScrollWindow(HWND hwnd, int dx, int dy, const RECT* rect, const RECT* clip);

HDC CreateCompatibleDC(HDC dc);
BOOL DeleteDC(HDC dc);
int SaveDC(HDC dc);
BOOL RestoreDC(HDC dc, int saved);
HBITMAP CreateCompatibleBitmap(HDC dc, int width, int height);
HBITMAP CreateBitmap(int width, int height, UINT planes, UINT bits_per_pixel, const void* bits);
HBITMAP CreateDIBSection(HDC dc, const BITMAPINFO* info, UINT usage, void** bits, HANDLE section,
                         DWORD offset);
int GetDIBits(HDC dc, HBITMAP bitmap, UINT start, UINT lines, LPVOID bits, LPBITMAPINFO info,
              UINT usage);
int SetDIBits(HDC dc, HBITMAP bitmap, UINT start, UINT lines, const void* bits,
              const BITMAPINFO* info, UINT usage);
int SetDIBitsToDevice(HDC dc, int x, int y, DWORD width, DWORD height, int src_x, int src_y,
                      UINT start, UINT lines, const void* bits, const BITMAPINFO* info, UINT usage);
int StretchDIBits(HDC dc, int x, int y, int width, int height, int src_x, int src_y,
                  int src_width, int src_height, const void* bits, const BITMAPINFO* info,
                  UINT usage, DWORD rop);
HBITMAP LoadBitmapA(HINSTANCE instance, LPCSTR name);
HANDLE LoadImageA(HINSTANCE instance, LPCSTR name, UINT type, int cx, int cy, UINT flags);
HGDIOBJ SelectObject(HDC dc, HGDIOBJ object);
BOOL DeleteObject(HGDIOBJ object);
int GetObjectA(HANDLE object, int size, LPVOID data);
DWORD GetObjectType(HGDIOBJ object);
HGDIOBJ GetStockObject(int index);
HGDIOBJ GetCurrentObject(HDC dc, UINT type);
HPEN CreatePen(int style, int width, COLORREF color);
HPEN CreatePenIndirect(const LOGPEN* pen);
HPEN ExtCreatePen(DWORD style, DWORD width, const LOGBRUSH* brush, DWORD style_count,
                  const DWORD* style_array);
HBRUSH CreateSolidBrush(COLORREF color);
HBRUSH CreateBrushIndirect(const LOGBRUSH* brush);
HBRUSH CreatePatternBrush(HBITMAP bitmap);
HFONT CreateFontA(int height, int width, int escapement, int orientation, int weight,
                  DWORD italic, DWORD underline, DWORD strike_out, DWORD char_set,
                  DWORD out_precision, DWORD clip_precision, DWORD quality,
                  DWORD pitch_and_family, LPCSTR face);
HFONT CreateFontIndirectA(const LOGFONTA* font);
HRGN CreateRectRgn(int left, int top, int right, int bottom);
HRGN CreateRectRgnIndirect(const RECT* rect);
int CombineRgn(HRGN dst, HRGN a, HRGN b, int mode);
int GetRgnBox(HRGN region, LPRECT rect);
DWORD GetRegionData(HRGN region, DWORD size, LPRGNDATA data);
BOOL PtInRegion(HRGN region, int x, int y);
int SelectClipRgn(HDC dc, HRGN region);
int ExtSelectClipRgn(HDC dc, HRGN region, int mode);
int GetClipRgn(HDC dc, HRGN region);
int GetClipBox(HDC dc, LPRECT rect);
int IntersectClipRect(HDC dc, int left, int top, int right, int bottom);
int ExcludeClipRect(HDC dc, int left, int top, int right, int bottom);
BOOL RectVisible(HDC dc, const RECT* rect);

COLORREF SetTextColor(HDC dc, COLORREF color);
COLORREF GetTextColor(HDC dc);
COLORREF SetBkColor(HDC dc, COLORREF color);
COLORREF GetBkColor(HDC dc);
int SetBkMode(HDC dc, int mode);
int GetBkMode(HDC dc);
int SetROP2(HDC dc, int rop2);
int GetROP2(HDC dc);
int SetPolyFillMode(HDC dc, int mode);
int SetStretchBltMode(HDC dc, int mode);
int SetMapMode(HDC dc, int mode);
UINT SetTextAlign(HDC dc, UINT align);
BOOL SetBrushOrgEx(HDC dc, int x, int y, LPPOINT previous);
BOOL SetViewportOrgEx(HDC dc, int x, int y, LPPOINT previous);
BOOL GetViewportOrgEx(HDC dc, LPPOINT point);
BOOL SetWindowOrgEx(HDC dc, int x, int y, LPPOINT previous);
int SetGraphicsMode(HDC dc, int mode);
int GetGraphicsMode(HDC dc);
BOOL SetWorldTransform(HDC dc, const XFORM* xf);
BOOL GetWorldTransform(HDC dc, XFORM* xf);
BOOL ModifyWorldTransform(HDC dc, const XFORM* xf, DWORD mode);
BOOL LPtoDP(HDC dc, LPPOINT points, int count);
BOOL DPtoLP(HDC dc, LPPOINT points, int count);
BOOL GdiFlush();
int GetDeviceCaps(HDC dc, int index);

BOOL MoveToEx(HDC dc, int x, int y, LPPOINT previous);
BOOL LineTo(HDC dc, int x, int y);
BOOL Polyline(HDC dc, const POINT* points, int count);
BOOL PolyBezier(HDC dc, const POINT* points, DWORD count);
BOOL Polygon(HDC dc, const POINT* points, int count);
BOOL Rectangle(HDC dc, int left, int top, int right, int bottom);
BOOL RoundRect(HDC dc, int left, int top, int right, int bottom, int width, int height);
BOOL Ellipse(HDC dc, int left, int top, int right, int bottom);
BOOL Arc(HDC dc, int left, int top, int right, int bottom, int x1, int y1, int x2, int y2);
BOOL Pie(HDC dc, int left, int top, int right, int bottom, int x1, int y1, int x2, int y2);
BOOL Chord(HDC dc, int left, int top, int right, int bottom, int x1, int y1, int x2, int y2);
int FillRect(HDC dc, const RECT* rect, HBRUSH brush);
int FrameRect(HDC dc, const RECT* rect, HBRUSH brush);
BOOL InvertRect(HDC dc, const RECT* rect);
BOOL DrawFocusRect(HDC dc, const RECT* rect);
BOOL PatBlt(HDC dc, int x, int y, int width, int height, DWORD rop);
COLORREF SetPixel(HDC dc, int x, int y, COLORREF color);
COLORREF GetPixel(HDC dc, int x, int y);
BOOL ExtFloodFill(HDC dc, int x, int y, COLORREF color, UINT type);
BOOL FloodFill(HDC dc, int x, int y, COLORREF color);
BOOL BitBlt(HDC dst, int x, int y, int width, int height, HDC src, int src_x, int src_y,
            DWORD rop);
BOOL StretchBlt(HDC dst, int x, int y, int width, int height, HDC src, int src_x, int src_y,
                int src_width, int src_height, DWORD rop);
BOOL MaskBlt(HDC dst, int x, int y, int width, int height, HDC src, int src_x, int src_y,
             HBITMAP mask, int mask_x, int mask_y, DWORD rop);
BOOL AlphaBlend(HDC dst, int x, int y, int width, int height, HDC src, int src_x, int src_y,
                int src_width, int src_height, BLENDFUNCTION blend);
BOOL TransparentBlt(HDC dst, int x, int y, int width, int height, HDC src, int src_x, int src_y,
                    int src_width, int src_height, UINT transparent);

BOOL TextOutA(HDC dc, int x, int y, LPCSTR text, int count);
BOOL ExtTextOutA(HDC dc, int x, int y, UINT options, const RECT* rect, LPCSTR text, UINT count,
                 const INT* dx);
int DrawTextA(HDC dc, LPCSTR text, int count, LPRECT rect, UINT format);
BOOL GetTextExtentPoint32A(HDC dc, LPCSTR text, int count, LPSIZE size);
BOOL GetTextMetricsA(HDC dc, TEXTMETRICA* metrics);

BOOL SetRect(LPRECT rect, int left, int top, int right, int bottom);
BOOL SetRectEmpty(LPRECT rect);
BOOL CopyRect(LPRECT dst, const RECT* src);
BOOL InflateRect(LPRECT rect, int dx, int dy);
BOOL OffsetRect(LPRECT rect, int dx, int dy);
BOOL IntersectRect(LPRECT dst, const RECT* a, const RECT* b);
BOOL UnionRect(LPRECT dst, const RECT* a, const RECT* b);
BOOL IsRectEmpty(const RECT* rect);
BOOL PtInRect(const RECT* rect, POINT point);
BOOL EqualRect(const RECT* a, const RECT* b);

#define FindWindowEx FindWindowExA
#define RegisterClass RegisterClassA
#define UnregisterClass UnregisterClassA
#define GetClassInfo GetClassInfoA
#define GetClassName GetClassNameA
#define CreateWindowEx CreateWindowExA
#define CreateWindowA(cls, name, style, x, y, w, h, parent, menu, inst, param)                     \
  CreateWindowExA(0, cls, name, style, x, y, w, h, parent, menu, inst, param)
#define CreateWindow CreateWindowA
#define GetWindowLong GetWindowLongA
#define SetWindowLong SetWindowLongA
#define GetWindowLongPtr GetWindowLongPtrA
#define SetWindowLongPtr SetWindowLongPtrA
#define SetProp SetPropA
#define GetProp GetPropA
#define RemoveProp RemovePropA
#define SetWindowText SetWindowTextA
#define GetWindowText GetWindowTextA
#define GetWindowTextLength GetWindowTextLengthA
#define DefWindowProc DefWindowProcA
#define CallWindowProc CallWindowProcA
#define SendMessage SendMessageA
#define PostMessage PostMessageA
#define PeekMessage PeekMessageA
#define GetMessage GetMessageA
#define DispatchMessage DispatchMessageA
#define MessageBox MessageBoxA
#define LoadCursor LoadCursorA
#define AppendMenu AppendMenuA
#define InsertMenu InsertMenuA
#define SetMenuItemInfo SetMenuItemInfoA
#define GetMenuItemInfo GetMenuItemInfoA
#define GetOpenFileName GetOpenFileNameA
#define GetSaveFileName GetSaveFileNameA
#define LoadBitmap LoadBitmapA
#define LoadImage LoadImageA
#define GetObject GetObjectA
#define CreateFont CreateFontA
#define CreateFontIndirect CreateFontIndirectA
#define TextOut TextOutA
#define ExtTextOut ExtTextOutA
#define DrawText DrawTextA
#define GetTextExtentPoint32 GetTextExtentPoint32A
#define GetTextMetrics GetTextMetricsA
#define LoadLibrary LoadLibraryA
#define GetModuleHandle GetModuleHandleA
#define GetModuleFileName GetModuleFileNameA
#define FindResource FindResourceA
#define GetEnvironmentVariable GetEnvironmentVariableA
#define SetEnvironmentVariable SetEnvironmentVariableA
#define GetFileAttributes GetFileAttributesA
#define CreateDirectory CreateDirectoryA
#define RemoveDirectory RemoveDirectoryA
#define DeleteFile DeleteFileA
#define MoveFile MoveFileA
#define CreateFile CreateFileA
#define FindFirstFile FindFirstFileA
#define FindNextFile FindNextFileA
#define GetCurrentDirectory GetCurrentDirectoryA
#define SetCurrentDirectory SetCurrentDirectoryA
#define GetTempPath GetTempPathA
#define SHGetFolderPath SHGetFolderPathA
#define DragQueryFile DragQueryFileA
#define ShellExecute ShellExecuteA
#define GetVersionEx GetVersionExA
#define OutputDebugString OutputDebugStringA
#define lstrlen lstrlenA
#define lstrcpy lstrcpyA
#define lstrcpyn lstrcpynA
#define lstrcat lstrcatA
#define lstrcmp lstrcmpA
#define lstrcmpi lstrcmpiA
#define wsprintf wsprintfA

#ifndef _MSC_VER
int _stricmp(const char* a, const char* b);
int _strnicmp(const char* a, const char* b, size_t n);
int _vsnprintf_s(char* dst, size_t size, size_t count, const char* format, va_list args);
int _snprintf_s(char* dst, size_t size, size_t count, const char* format, ...);
int _putenv_s(const char* name, const char* value);
#  define stricmp _stricmp
#  define strnicmp _strnicmp
#  define _snprintf snprintf
#  define _vsnprintf vsnprintf
#  define _strdup strdup
#endif

FILE* dtblkfix_win32_fopen(const char* file, const char* mode);
int dtblkfix_win32_vsnprintf(char* dst, size_t size, const char* format, const va_list args);

#endif
