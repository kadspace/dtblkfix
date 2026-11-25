#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstring>
#endif

#ifndef DTBLKFIX_SOFTWIN
#define DTBLKFIX_SOFTWIN 1
#endif

#ifndef STRICT
#define STRICT 1
#endif
#define WINAPI
#define CALLBACK
#define APIENTRY
#define __stdcall
#define __cdecl
#define FAR
#define NEAR
#define CONST const
#define VOID void
#define _TRUNCATE ((size_t)-1)

typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int INT;
typedef unsigned int UINT;
typedef short SHORT;
typedef unsigned short USHORT;
typedef char CHAR;
typedef wchar_t WCHAR;
typedef float FLOAT;
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t DWORD_PTR;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef int32_t HRESULT;
typedef WORD ATOM;
typedef DWORD COLORREF;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef char TCHAR;
typedef LPSTR LPTSTR;
typedef LPCSTR LPCTSTR;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef void* PVOID;
typedef BYTE* LPBYTE;
typedef DWORD* LPDWORD;
typedef LONG* LPLONG;
typedef BOOL* LPBOOL;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;

typedef void* HANDLE;
#define DECLARE_SOFT_HANDLE(name) typedef struct name##__* name
DECLARE_SOFT_HANDLE(HWND);
DECLARE_SOFT_HANDLE(HDC);
DECLARE_SOFT_HANDLE(HMENU);
DECLARE_SOFT_HANDLE(HINSTANCE);
DECLARE_SOFT_HANDLE(HICON);
DECLARE_SOFT_HANDLE(HKEY);
typedef HINSTANCE HMODULE;
typedef HICON HCURSOR;
typedef void* HGDIOBJ;
typedef HGDIOBJ HBITMAP;
typedef HGDIOBJ HBRUSH;
typedef HGDIOBJ HPEN;
typedef HGDIOBJ HFONT;
typedef HGDIOBJ HRGN;
typedef HGDIOBJ HPALETTE;
typedef HANDLE HGLOBAL;
typedef HANDLE HDROP;
typedef intptr_t(WINAPI* FARPROC)();
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define S_OK ((HRESULT)0)
#define NOERROR S_OK
#define S_FALSE ((HRESULT)1)
#define E_FAIL ((HRESULT)0x80004005)
#define E_NOTIMPL ((HRESULT)0x80004001)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define MAX_PATH 260

#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) ((BYTE)((rgb) >> 16))
#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define MAKELONG(a, b) ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define MAKEWPARAM(l, h) ((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELPARAM(l, h) ((LPARAM)(DWORD)MAKELONG(l, h))
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#define GET_WHEEL_DELTA_WPARAM(wParam) ((short)HIWORD(wParam))
#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCE MAKEINTRESOURCEA
#define IS_INTRESOURCE(r) ((((ULONG_PTR)(r)) >> 16) == 0)

typedef struct tagRECT { LONG left, top, right, bottom; } RECT, *PRECT, *LPRECT;
typedef const RECT* LPCRECT;
typedef struct tagPOINT { LONG x, y; } POINT, *PPOINT, *LPPOINT;
typedef struct tagSIZE { LONG cx, cy; } SIZE, *PSIZE, *LPSIZE;
typedef struct tagPOINTS { SHORT x, y; } POINTS;

typedef struct tagMSG {
  HWND hwnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD time;
  POINT pt;
} MSG, *LPMSG;

typedef struct tagPAINTSTRUCT {
  HDC hdc;
  BOOL fErase;
  RECT rcPaint;
  BOOL fRestore;
  BOOL fIncUpdate;
  BYTE rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct tagWNDCLASSA {
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
} WNDCLASSA, WNDCLASS, *LPWNDCLASSA;

typedef struct tagWNDCLASSEXA {
  UINT cbSize;
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
  HICON hIconSm;
} WNDCLASSEXA, WNDCLASSEX;

typedef struct tagCREATESTRUCTA {
  LPVOID lpCreateParams;
  HINSTANCE hInstance;
  HMENU hMenu;
  HWND hwndParent;
  int cy, cx, y, x;
  LONG style;
  LPCSTR lpszName;
  LPCSTR lpszClass;
  DWORD dwExStyle;
} CREATESTRUCTA, CREATESTRUCT, *LPCREATESTRUCT;

#define LF_FACESIZE 32
typedef struct tagLOGFONTA {
  LONG lfHeight;
  LONG lfWidth;
  LONG lfEscapement;
  LONG lfOrientation;
  LONG lfWeight;
  BYTE lfItalic;
  BYTE lfUnderline;
  BYTE lfStrikeOut;
  BYTE lfCharSet;
  BYTE lfOutPrecision;
  BYTE lfClipPrecision;
  BYTE lfQuality;
  BYTE lfPitchAndFamily;
  CHAR lfFaceName[LF_FACESIZE];
} LOGFONTA, LOGFONT, *PLOGFONTA, *LPLOGFONTA, *LPLOGFONT;

typedef struct tagMEASUREITEMSTRUCT {
  UINT CtlType;
  UINT CtlID;
  UINT itemID;
  UINT itemWidth;
  UINT itemHeight;
  ULONG_PTR itemData;
} MEASUREITEMSTRUCT, *LPMEASUREITEMSTRUCT;

typedef struct tagDRAWITEMSTRUCT {
  UINT CtlType;
  UINT CtlID;
  UINT itemID;
  UINT itemAction;
  UINT itemState;
  HWND hwndItem;
  HDC hDC;
  RECT rcItem;
  ULONG_PTR itemData;
} DRAWITEMSTRUCT, *LPDRAWITEMSTRUCT;
#define ODT_MENU 1
#define ODS_SELECTED 0x0001
#define ODS_GRAYED 0x0002
#define ODS_DISABLED 0x0004
#define ODS_CHECKED 0x0008
#define ODA_DRAWENTIRE 0x0001
#define ODA_SELECT 0x0002

typedef struct tagTEXTMETRICA {
  LONG tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading, tmAveCharWidth,
      tmMaxCharWidth, tmWeight, tmOverhang, tmDigitizedAspectX, tmDigitizedAspectY;
  BYTE tmFirstChar, tmLastChar, tmDefaultChar, tmBreakChar, tmItalic, tmUnderlined, tmStruckOut,
      tmPitchAndFamily, tmCharSet;
} TEXTMETRICA, TEXTMETRIC, *LPTEXTMETRICA, *LPTEXTMETRIC;

typedef struct tagLOGPEN { UINT lopnStyle; POINT lopnWidth; COLORREF lopnColor; } LOGPEN, *LPLOGPEN;
typedef struct tagLOGBRUSH { UINT lbStyle; COLORREF lbColor; ULONG_PTR lbHatch; } LOGBRUSH, *LPLOGBRUSH;

typedef struct tagBITMAP {
  LONG bmType;
  LONG bmWidth;
  LONG bmHeight;
  LONG bmWidthBytes;
  WORD bmPlanes;
  WORD bmBitsPixel;
  LPVOID bmBits;
} BITMAP, *PBITMAP, *LPBITMAP;

typedef struct tagRGBQUAD { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; } RGBQUAD;

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD bmiColors[1];
} BITMAPINFO, *PBITMAPINFO, *LPBITMAPINFO;

typedef struct tagCIEXYZ { LONG ciexyzX, ciexyzY, ciexyzZ; } CIEXYZ;
typedef struct tagCIEXYZTRIPLE { CIEXYZ ciexyzRed, ciexyzGreen, ciexyzBlue; } CIEXYZTRIPLE;

typedef struct {
  DWORD bV5Size;
  LONG bV5Width;
  LONG bV5Height;
  WORD bV5Planes;
  WORD bV5BitCount;
  DWORD bV5Compression;
  DWORD bV5SizeImage;
  LONG bV5XPelsPerMeter;
  LONG bV5YPelsPerMeter;
  DWORD bV5ClrUsed;
  DWORD bV5ClrImportant;
  DWORD bV5RedMask;
  DWORD bV5GreenMask;
  DWORD bV5BlueMask;
  DWORD bV5AlphaMask;
  DWORD bV5CSType;
  CIEXYZTRIPLE bV5Endpoints;
  DWORD bV5GammaRed;
  DWORD bV5GammaGreen;
  DWORD bV5GammaBlue;
  DWORD bV5Intent;
  DWORD bV5ProfileData;
  DWORD bV5ProfileSize;
  DWORD bV5Reserved;
} BITMAPV5HEADER, *LPBITMAPV5HEADER, *PBITMAPV5HEADER;

typedef struct tagXFORM { FLOAT eM11, eM12, eM21, eM22, eDx, eDy; } XFORM, *PXFORM, *LPXFORM;

typedef struct _BLENDFUNCTION {
  BYTE BlendOp;
  BYTE BlendFlags;
  BYTE SourceConstantAlpha;
  BYTE AlphaFormat;
} BLENDFUNCTION, *PBLENDFUNCTION;

typedef struct _OSVERSIONINFOA {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  CHAR szCSDVersion[128];
} OSVERSIONINFOA, OSVERSIONINFO, *LPOSVERSIONINFOA, *LPOSVERSIONINFO;

typedef struct _OSVERSIONINFOEXA {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  CHAR szCSDVersion[128];
  WORD wServicePackMajor;
  WORD wServicePackMinor;
  WORD wSuiteMask;
  BYTE wProductType;
  BYTE wReserved;
} OSVERSIONINFOEXA, OSVERSIONINFOEX, *LPOSVERSIONINFOEXA;

typedef struct _SYSTEMTIME {
  WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME;

typedef struct tagTRACKMOUSEEVENT {
  DWORD cbSize;
  DWORD dwFlags;
  HWND hwndTrack;
  DWORD dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

typedef struct _GUID {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t Data4[8];
} GUID, IID, CLSID;
typedef const GUID& REFGUID;
typedef const IID& REFIID;
typedef const CLSID& REFCLSID;

#define BI_RGB 0
#define BI_BITFIELDS 3
#define DIB_RGB_COLORS 0
#define LR_CREATEDIBSECTION 0x2000
#define LR_DEFAULTCOLOR 0
#define IMAGE_BITMAP 0
#define AC_SRC_OVER 0
#define AC_SRC_ALPHA 1

#define SRCCOPY 0x00CC0020
#define SRCPAINT 0x00EE0086
#define SRCAND 0x008800C6
#define SRCINVERT 0x00660046
#define SRCERASE 0x00440328
#define NOTSRCCOPY 0x00330008
#define NOTSRCERASE 0x001100A6
#define MERGEPAINT 0x00BB0226
#define PATCOPY 0x00F00021
#define PATINVERT 0x005A0049
#define DSTINVERT 0x00550009
#define BLACKNESS 0x00000042
#define WHITENESS 0x00FF0062

#define R2_BLACK 1
#define R2_NOT 6
#define R2_XORPEN 7
#define R2_NOTXORPEN 10
#define R2_MERGEPEN 15
#define R2_MASKPEN 9
#define R2_NOTCOPYPEN 4
#define R2_NOP 11
#define R2_COPYPEN 13
#define R2_WHITE 16

#define TRANSPARENT 1
#define OPAQUE 2
#define GM_COMPATIBLE 1
#define GM_ADVANCED 2
#define MWT_IDENTITY 1
#define MWT_LEFTMULTIPLY 2
#define MWT_RIGHTMULTIPLY 3
#define BLACKONWHITE 1
#define WHITEONBLACK 2
#define COLORONCOLOR 3
#define HALFTONE 4
#define STRETCH_DELETESCANS COLORONCOLOR
#define ALTERNATE 1
#define WINDING 2
#define FLOODFILLBORDER 0
#define FLOODFILLSURFACE 1
#define CLR_INVALID 0xFFFFFFFF
#define RGN_AND 1
#define RGN_OR 2
#define RGN_XOR 3
#define RGN_DIFF 4
#define RGN_COPY 5
#define ERROR 0
#define NULLREGION 1
#define SIMPLEREGION 2
#define COMPLEXREGION 3

#define PS_SOLID 0
#define PS_DASH 1
#define PS_DOT 2
#define PS_DASHDOT 3
#define PS_DASHDOTDOT 4
#define PS_NULL 5
#define PS_INSIDEFRAME 6
#define PS_USERSTYLE 7
#define PS_ALTERNATE 8
#define PS_STYLE_MASK 0x0000000F
#define PS_ENDCAP_ROUND 0x00000000
#define PS_ENDCAP_SQUARE 0x00000100
#define PS_ENDCAP_FLAT 0x00000200
#define PS_JOIN_ROUND 0x00000000
#define PS_JOIN_BEVEL 0x00001000
#define PS_JOIN_MITER 0x00002000
#define PS_COSMETIC 0x00000000
#define PS_GEOMETRIC 0x00010000

#define BS_SOLID 0
#define BS_NULL 1
#define BS_HOLLOW BS_NULL
#define BS_HATCHED 2

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
#define DEFAULT_GUI_FONT 17
#define DC_BRUSH 18
#define DC_PEN 19

#define OBJ_PEN 1
#define OBJ_BRUSH 2
#define OBJ_DC 3
#define OBJ_FONT 6
#define OBJ_BITMAP 7
#define OBJ_REGION 8

#define FW_DONTCARE 0
#define FW_THIN 100
#define FW_LIGHT 300
#define FW_NORMAL 400
#define FW_MEDIUM 500
#define FW_SEMIBOLD 600
#define FW_BOLD 700
#define FW_HEAVY 900
#define ANSI_CHARSET 0
#define DEFAULT_CHARSET 1
#define SYMBOL_CHARSET 2
#define OUT_DEFAULT_PRECIS 0
#define OUT_TT_PRECIS 4
#define OUT_STRING_PRECIS 1
#define OUT_CHARACTER_PRECIS 2
#define OUT_STROKE_PRECIS 3
#define OUT_TT_ONLY_PRECIS 7
#define CLIP_DEFAULT_PRECIS 0
#define CLIP_CHARACTER_PRECIS 1
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
#define FF_DONTCARE 0
#define FF_ROMAN 16
#define FF_SWISS 32
#define FF_MODERN 48
#define FF_SCRIPT 64
#define FF_DECORATIVE 80

#define DT_TOP 0x0
#define DT_LEFT 0x0
#define DT_CENTER 0x1
#define DT_RIGHT 0x2
#define DT_VCENTER 0x4
#define DT_BOTTOM 0x8
#define DT_WORDBREAK 0x10
#define DT_SINGLELINE 0x20
#define DT_EXPANDTABS 0x40
#define DT_NOCLIP 0x100
#define DT_CALCRECT 0x400
#define DT_NOPREFIX 0x800
#define DT_END_ELLIPSIS 0x8000

#define WM_NULL 0x0000
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
#define WM_ACTIVATE 0x0006
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_ENABLE 0x000A
#define WM_SETTEXT 0x000C
#define WM_GETTEXT 0x000D
#define WM_GETTEXTLENGTH 0x000E
#define WM_PAINT 0x000F
#define WM_CLOSE 0x0010
#define WM_ERASEBKGND 0x0014
#define WM_SHOWWINDOW 0x0018
#define WM_SETCURSOR 0x0020
#define WM_MOUSEACTIVATE 0x0021
#define WM_SETFONT 0x0030
#define WM_GETFONT 0x0031
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED 0x0047
#define WM_NCCREATE 0x0081
#define WM_NCDESTROY 0x0082
#define WM_NCHITTEST 0x0084
#define WM_NCPAINT 0x0085
#define WM_GETDLGCODE 0x0087
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define WM_COMMAND 0x0111
#define WM_TIMER 0x0113
#define WM_CTLCOLOREDIT 0x0133
#define WM_DRAWITEM 0x002B
#define WM_MEASUREITEM 0x002C
#define WM_MENUSELECT 0x011F
#define WM_ENTERMENULOOP 0x0211
#define WM_EXITMENULOOP 0x0212
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
#define WM_MOUSEWHEEL 0x020A
#define WM_MOUSEHWHEEL 0x020E
#define WM_MOUSELAST 0x020E
#define WM_CAPTURECHANGED 0x0215
#define WM_DROPFILES 0x0233
#define WM_MOUSEHOVER 0x02A1
#define WM_MOUSELEAVE 0x02A3
#define WM_USER 0x0400

#define MK_LBUTTON 0x0001
#define MK_RBUTTON 0x0002
#define MK_SHIFT 0x0004
#define MK_CONTROL 0x0008
#define MK_MBUTTON 0x0010
#define MK_XBUTTON1 0x0020
#define MK_XBUTTON2 0x0040
#define WHEEL_DELTA 120
#define TME_HOVER 0x00000001
#define TME_LEAVE 0x00000002
#define HOVER_DEFAULT 0xFFFFFFFF

#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_CANCEL 0x03
#define VK_MBUTTON 0x04
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
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
#define VK_PRINT 0x2A
#define VK_EXECUTE 0x2B
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_HELP 0x2F
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
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
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
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
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5

#define WS_OVERLAPPED 0x00000000L
#define WS_POPUP 0x80000000L
#define WS_CHILD 0x40000000L
#define WS_MINIMIZE 0x20000000L
#define WS_VISIBLE 0x10000000L
#define WS_DISABLED 0x08000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#define WS_MAXIMIZE 0x01000000L
#define WS_CAPTION 0x00C00000L
#define WS_BORDER 0x00800000L
#define WS_DLGFRAME 0x00400000L
#define WS_VSCROLL 0x00200000L
#define WS_HSCROLL 0x00100000L
#define WS_SYSMENU 0x00080000L
#define WS_THICKFRAME 0x00040000L
#define WS_GROUP 0x00020000L
#define WS_TABSTOP 0x00010000L
#define WS_EX_TOPMOST 0x00000008L
#define WS_EX_TRANSPARENT 0x00000020L
#define WS_EX_TOOLWINDOW 0x00000080L
#define WS_EX_CLIENTEDGE 0x00000200L
#define WS_EX_NOACTIVATE 0x08000000L
#define WS_EX_LAYERED 0x00080000L
#define ES_LEFT 0x0000L
#define ES_CENTER 0x0001L
#define ES_RIGHT 0x0002L
#define ES_MULTILINE 0x0004L
#define ES_AUTOHSCROLL 0x0080L
#define ES_NUMBER 0x2000L
#define EM_SETSEL 0x00B1
#define EM_GETSEL 0x00B0
#define EM_SETMARGINS 0x00D3
#define EC_LEFTMARGIN 0x0001
#define EC_RIGHTMARGIN 0x0002
#define EM_LIMITTEXT 0x00C5
#define EM_SETLIMITTEXT EM_LIMITTEXT
#define CS_VREDRAW 0x0001
#define CS_HREDRAW 0x0002
#define CS_DBLCLKS 0x0008
#define CS_OWNDC 0x0020
#define CS_GLOBALCLASS 0x4000
#define CW_USEDEFAULT ((int)0x80000000)
#define DLGC_WANTALLKEYS 0x0004
#define DLGC_WANTCHARS 0x0080
#define DLGC_HASSETSEL 0x0008

#define GWL_WNDPROC (-4)
#define GWL_HINSTANCE (-6)
#define GWL_STYLE (-16)
#define GWL_EXSTYLE (-20)
#define GWL_USERDATA (-21)
#define GWL_ID (-12)
#define GWLP_WNDPROC (-4)
#define GWLP_HINSTANCE (-6)
#define GWLP_USERDATA (-21)
#define GWLP_ID (-12)

#define SWP_NOSIZE 0x0001
#define SWP_NOMOVE 0x0002
#define SWP_NOZORDER 0x0004
#define SWP_NOREDRAW 0x0008
#define SWP_NOACTIVATE 0x0010
#define SWP_FRAMECHANGED 0x0020
#define SWP_SHOWWINDOW 0x0040
#define SWP_HIDEWINDOW 0x0080
#define HWND_TOP ((HWND)0)
#define HWND_TOPMOST ((HWND)-1)
#define HWND_DESKTOP ((HWND)0)
#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_SHOW 5
#define SW_SHOWNA 8
#define SW_SHOWNOACTIVATE 4
#define RDW_INVALIDATE 0x0001
#define RDW_ERASE 0x0004
#define RDW_UPDATENOW 0x0100
#define RDW_ALLCHILDREN 0x0080
#define ULW_COLORKEY 0x00000001
#define ULW_ALPHA 0x00000002
#define ULW_OPAQUE 0x00000004
#define LWA_COLORKEY 0x00000001
#define LWA_ALPHA 0x00000002

#define PM_NOREMOVE 0x0000
#define PM_REMOVE 0x0001
#define PM_NOYIELD 0x0002

#define MF_STRING 0x00000000L
#define MF_ENABLED 0x00000000L
#define MF_UNCHECKED 0x00000000L
#define MF_BYCOMMAND 0x00000000L
#define MF_GRAYED 0x00000001L
#define MF_DISABLED 0x00000002L
#define MF_BITMAP 0x00000004L
#define MF_CHECKED 0x00000008L
#define MF_POPUP 0x00000010L
#define MF_MENUBARBREAK 0x00000020L
#define MF_MENUBREAK 0x00000040L
#define MF_OWNERDRAW 0x00000100L
#define MF_BYPOSITION 0x00000400L
#define MF_SEPARATOR 0x00000800L
#define TPM_LEFTBUTTON 0x0000L
#define TPM_RIGHTBUTTON 0x0002L
#define TPM_LEFTALIGN 0x0000L
#define TPM_CENTERALIGN 0x0004L
#define TPM_RIGHTALIGN 0x0008L
#define TPM_TOPALIGN 0x0000L
#define TPM_VCENTERALIGN 0x0010L
#define TPM_BOTTOMALIGN 0x0020L
#define TPM_NONOTIFY 0x0080L
#define TPM_RETURNCMD 0x0100L

#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define SM_CXVSCROLL 2
#define SM_CYHSCROLL 3
#define SM_CYCAPTION 4
#define SM_CXBORDER 5
#define SM_CYBORDER 6
#define SM_CXDOUBLECLK 36
#define SM_CYDOUBLECLK 37
#define SM_CXDRAG 68
#define SM_CYDRAG 69
#define SM_SWAPBUTTON 23
#define SM_CXFIXEDFRAME 7
#define SM_CYFIXEDFRAME 8
#define SM_CXFRAME 32
#define SM_CYFRAME 33
#define SM_CXEDGE 45
#define SM_CYEDGE 46
#define SM_CMOUSEBUTTONS 43
#define SM_CXVIRTUALSCREEN 78
#define SM_CYVIRTUALSCREEN 79
#define SM_XVIRTUALSCREEN 76
#define SM_YVIRTUALSCREEN 77

#define COLOR_SCROLLBAR 0
#define COLOR_BACKGROUND 1
#define COLOR_WINDOW 5
#define COLOR_WINDOWTEXT 8
#define COLOR_BTNFACE 15
#define COLOR_HIGHLIGHT 13
#define COLOR_HIGHLIGHTTEXT 14
#define COLOR_GRAYTEXT 17
#define COLOR_BTNTEXT 18
#define COLOR_3DFACE COLOR_BTNFACE

#define IDC_ARROW MAKEINTRESOURCE(32512)
#define IDC_IBEAM MAKEINTRESOURCE(32513)
#define IDC_WAIT MAKEINTRESOURCE(32514)
#define IDC_CROSS MAKEINTRESOURCE(32515)
#define IDC_UPARROW MAKEINTRESOURCE(32516)
#define IDC_SIZENWSE MAKEINTRESOURCE(32642)
#define IDC_SIZENESW MAKEINTRESOURCE(32643)
#define IDC_SIZEWE MAKEINTRESOURCE(32644)
#define IDC_SIZENS MAKEINTRESOURCE(32645)
#define IDC_SIZEALL MAKEINTRESOURCE(32646)
#define IDC_NO MAKEINTRESOURCE(32648)
#define IDC_HAND MAKEINTRESOURCE(32649)
#define IDC_APPSTARTING MAKEINTRESOURCE(32650)
#define IDC_HELP MAKEINTRESOURCE(32651)
#define IDI_APPLICATION MAKEINTRESOURCE(32512)

#define MB_OK 0x00000000L
#define MB_OKCANCEL 0x00000001L
#define MB_YESNO 0x00000004L
#define MB_ICONERROR 0x00000010L
#define MB_ICONWARNING 0x00000030L
#define MB_ICONINFORMATION 0x00000040L
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7

#define GMEM_MOVEABLE 0x0002
#define GMEM_ZEROINIT 0x0040
#define GHND (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define CF_TEXT 1
#define CF_HDROP 15
#define CP_ACP 0
#define CP_UTF8 65001
#define VER_PLATFORM_WIN32_NT 2
#define VER_PLATFORM_WIN32_WINDOWS 1
#define CSIDL_PERSONAL 0x0005
#define CSIDL_APPDATA 0x001a
#define CSIDL_LOCAL_APPDATA 0x001c
#define CSIDL_FLAG_CREATE 0x8000
#define SHGFP_TYPE_CURRENT 0
#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH 2
#define DLL_THREAD_DETACH 3
#define INFINITE 0xFFFFFFFF
#define DRAGDROP_S_DROP ((HRESULT)0x00040100L)
#define DROPEFFECT_NONE 0
#define DROPEFFECT_COPY 1
#define DROPEFFECT_MOVE 2
#define DROPEFFECT_LINK 4
#define DVASPECT_CONTENT 1
#define TYMED_HGLOBAL 1
#define CLSCTX_INPROC_SERVER 0x1

extern "C++" {

HDC WINAPI GetDC(HWND);
HDC WINAPI GetWindowDC(HWND);
int WINAPI ReleaseDC(HWND, HDC);
HDC WINAPI CreateCompatibleDC(HDC);
BOOL WINAPI DeleteDC(HDC);
int WINAPI SaveDC(HDC);
BOOL WINAPI RestoreDC(HDC, int);
HGDIOBJ WINAPI SelectObject(HDC, HGDIOBJ);
BOOL WINAPI DeleteObject(HGDIOBJ);
HGDIOBJ WINAPI GetStockObject(int);
HGDIOBJ WINAPI GetCurrentObject(HDC, UINT);
int WINAPI GetObjectA(HANDLE, int, LPVOID);
#define GetObject GetObjectA
DWORD WINAPI GetObjectType(HGDIOBJ);

HBITMAP WINAPI CreateCompatibleBitmap(HDC, int, int);
HBITMAP WINAPI CreateBitmap(int, int, UINT, UINT, const void*);
HBITMAP WINAPI CreateDIBSection(HDC, const BITMAPINFO*, UINT, void**, HANDLE, DWORD);
int WINAPI GetDIBits(HDC, HBITMAP, UINT, UINT, LPVOID, LPBITMAPINFO, UINT);
int WINAPI SetDIBits(HDC, HBITMAP, UINT, UINT, const void*, const BITMAPINFO*, UINT);
LONG WINAPI GetBitmapBits(HBITMAP, LONG, LPVOID);
HBITMAP WINAPI LoadBitmapA(HINSTANCE, LPCSTR);
#define LoadBitmap LoadBitmapA
HANDLE WINAPI LoadImageA(HINSTANCE, LPCSTR, UINT, int, int, UINT);
#define LoadImage LoadImageA

HPEN WINAPI CreatePen(int, int, COLORREF);
HPEN WINAPI CreatePenIndirect(const LOGPEN*);
HPEN WINAPI ExtCreatePen(DWORD, DWORD, const LOGBRUSH*, DWORD, const DWORD*);
HBRUSH WINAPI CreateSolidBrush(COLORREF);
HBRUSH WINAPI CreateBrushIndirect(const LOGBRUSH*);
HBRUSH WINAPI GetSysColorBrush(int);
HFONT WINAPI CreateFontIndirectA(const LOGFONTA*);
#define CreateFontIndirect CreateFontIndirectA
HFONT WINAPI CreateFontA(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCSTR);
#define CreateFont CreateFontA
COLORREF WINAPI SetDCBrushColor(HDC, COLORREF);
COLORREF WINAPI SetDCPenColor(HDC, COLORREF);

HRGN WINAPI CreateRectRgn(int, int, int, int);
HRGN WINAPI CreateRectRgnIndirect(const RECT*);
HRGN WINAPI CreateRoundRectRgn(int, int, int, int, int, int);
HRGN WINAPI CreateEllipticRgn(int, int, int, int);
int WINAPI CombineRgn(HRGN, HRGN, HRGN, int);
int WINAPI SelectClipRgn(HDC, HRGN);
int WINAPI ExtSelectClipRgn(HDC, HRGN, int);
int WINAPI IntersectClipRect(HDC, int, int, int, int);
int WINAPI ExcludeClipRect(HDC, int, int, int, int);
int WINAPI GetClipBox(HDC, LPRECT);
int WINAPI GetClipRgn(HDC, HRGN);
int WINAPI GetRgnBox(HRGN, LPRECT);
BOOL WINAPI PtInRegion(HRGN, int, int);
BOOL WINAPI FillRgn(HDC, HRGN, HBRUSH);
int WINAPI OffsetRgn(HRGN, int, int);

int WINAPI SetGraphicsMode(HDC, int);
int WINAPI GetGraphicsMode(HDC);
BOOL WINAPI SetWorldTransform(HDC, const XFORM*);
BOOL WINAPI GetWorldTransform(HDC, LPXFORM);
BOOL WINAPI ModifyWorldTransform(HDC, const XFORM*, DWORD);
BOOL WINAPI LPtoDP(HDC, LPPOINT, int);
BOOL WINAPI DPtoLP(HDC, LPPOINT, int);
BOOL WINAPI SetViewportOrgEx(HDC, int, int, LPPOINT);
BOOL WINAPI GetViewportOrgEx(HDC, LPPOINT);
BOOL WINAPI SetWindowOrgEx(HDC, int, int, LPPOINT);
BOOL WINAPI OffsetViewportOrgEx(HDC, int, int, LPPOINT);
BOOL WINAPI SetBrushOrgEx(HDC, int, int, LPPOINT);

COLORREF WINAPI SetTextColor(HDC, COLORREF);
COLORREF WINAPI GetTextColor(HDC);
COLORREF WINAPI SetBkColor(HDC, COLORREF);
COLORREF WINAPI GetBkColor(HDC);
int WINAPI SetBkMode(HDC, int);
int WINAPI GetBkMode(HDC);
int WINAPI SetROP2(HDC, int);
int WINAPI GetROP2(HDC);
int WINAPI SetStretchBltMode(HDC, int);
int WINAPI SetPolyFillMode(HDC, int);
UINT WINAPI SetTextAlign(HDC, UINT);

BOOL WINAPI MoveToEx(HDC, int, int, LPPOINT);
BOOL WINAPI LineTo(HDC, int, int);
BOOL WINAPI Polyline(HDC, const POINT*, int);
BOOL WINAPI PolylineTo(HDC, const POINT*, DWORD);
BOOL WINAPI Polygon(HDC, const POINT*, int);
BOOL WINAPI PolyPolygon(HDC, const POINT*, const INT*, int);
BOOL WINAPI Rectangle(HDC, int, int, int, int);
BOOL WINAPI RoundRect(HDC, int, int, int, int, int, int);
BOOL WINAPI Ellipse(HDC, int, int, int, int);
BOOL WINAPI Arc(HDC, int, int, int, int, int, int, int, int);
BOOL WINAPI Pie(HDC, int, int, int, int, int, int, int, int);
BOOL WINAPI Chord(HDC, int, int, int, int, int, int, int, int);
int WINAPI FillRect(HDC, const RECT*, HBRUSH);
int WINAPI FrameRect(HDC, const RECT*, HBRUSH);
BOOL WINAPI InvertRect(HDC, const RECT*);
COLORREF WINAPI SetPixel(HDC, int, int, COLORREF);
BOOL WINAPI SetPixelV(HDC, int, int, COLORREF);
COLORREF WINAPI GetPixel(HDC, int, int);
BOOL WINAPI ExtFloodFill(HDC, int, int, COLORREF, UINT);
BOOL WINAPI FloodFill(HDC, int, int, COLORREF);
BOOL WINAPI GdiFlush();

BOOL WINAPI BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);
BOOL WINAPI StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
BOOL WINAPI PatBlt(HDC, int, int, int, int, DWORD);
BOOL WINAPI AlphaBlend(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
BOOL WINAPI TransparentBlt(HDC, int, int, int, int, HDC, int, int, int, int, UINT);
BOOL WINAPI MaskBlt(HDC, int, int, int, int, HDC, int, int, HBITMAP, int, int, DWORD);

BOOL WINAPI TextOutA(HDC, int, int, LPCSTR, int);
#define TextOut TextOutA
BOOL WINAPI ExtTextOutA(HDC, int, int, UINT, const RECT*, LPCSTR, UINT, const INT*);
#define ExtTextOut ExtTextOutA
int WINAPI DrawTextA(HDC, LPCSTR, int, LPRECT, UINT);
#define DrawText DrawTextA
BOOL WINAPI GetTextExtentPoint32A(HDC, LPCSTR, int, LPSIZE);
#define GetTextExtentPoint32 GetTextExtentPoint32A
#define GetTextExtentPointA GetTextExtentPoint32A
BOOL WINAPI GetTextMetricsA(HDC, LPTEXTMETRICA);
#define GetTextMetrics GetTextMetricsA
int WINAPI GetDeviceCaps(HDC, int);
#define LOGPIXELSX 88
#define LOGPIXELSY 90
#define BITSPIXEL 12
#define HORZRES 8
#define VERTRES 10

ATOM WINAPI RegisterClassA(const WNDCLASSA*);
#define RegisterClass RegisterClassA
ATOM WINAPI RegisterClassExA(const WNDCLASSEXA*);
#define RegisterClassEx RegisterClassExA
BOOL WINAPI UnregisterClassA(LPCSTR, HINSTANCE);
#define UnregisterClass UnregisterClassA
BOOL WINAPI GetClassInfoA(HINSTANCE, LPCSTR, LPWNDCLASSA);
int WINAPI GetClassNameA(HWND, LPSTR, int);
#define GetClassName GetClassNameA
HWND WINAPI CreateWindowExA(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
#define CreateWindowEx CreateWindowExA
#define CreateWindowA(c, n, s, x, y, w, h, p, m, i, l) CreateWindowExA(0, c, n, s, x, y, w, h, p, m, i, l)
#define CreateWindow CreateWindowA
BOOL WINAPI DestroyWindow(HWND);
LRESULT WINAPI DefWindowProcA(HWND, UINT, WPARAM, LPARAM);
#define DefWindowProc DefWindowProcA
LRESULT WINAPI CallWindowProcA(WNDPROC, HWND, UINT, WPARAM, LPARAM);
#define CallWindowProc CallWindowProcA
LRESULT WINAPI SendMessageA(HWND, UINT, WPARAM, LPARAM);
#define SendMessage SendMessageA
BOOL WINAPI PostMessageA(HWND, UINT, WPARAM, LPARAM);
#define PostMessage PostMessageA
BOOL WINAPI PeekMessageA(LPMSG, HWND, UINT, UINT, UINT);
#define PeekMessage PeekMessageA
BOOL WINAPI GetMessageA(LPMSG, HWND, UINT, UINT);
#define GetMessage GetMessageA
LRESULT WINAPI DispatchMessageA(const MSG*);
#define DispatchMessage DispatchMessageA
BOOL WINAPI TranslateMessage(const MSG*);
LONG WINAPI GetMessageTime();
DWORD WINAPI GetMessagePos();
LONG WINAPI GetWindowLongA(HWND, int);
#define GetWindowLong GetWindowLongA
LONG WINAPI SetWindowLongA(HWND, int, LONG);
#define SetWindowLong SetWindowLongA
LONG_PTR WINAPI GetWindowLongPtrA(HWND, int);
#define GetWindowLongPtr GetWindowLongPtrA
LONG_PTR WINAPI SetWindowLongPtrA(HWND, int, LONG_PTR);
#define SetWindowLongPtr SetWindowLongPtrA
BOOL WINAPI SetPropA(HWND, LPCSTR, HANDLE);
#define SetProp SetPropA
HANDLE WINAPI GetPropA(HWND, LPCSTR);
#define GetProp GetPropA
HANDLE WINAPI RemovePropA(HWND, LPCSTR);
#define RemoveProp RemovePropA
BOOL WINAPI SetWindowTextA(HWND, LPCSTR);
#define SetWindowText SetWindowTextA
int WINAPI GetWindowTextA(HWND, LPSTR, int);
#define GetWindowText GetWindowTextA
int WINAPI GetWindowTextLengthA(HWND);
#define GetWindowTextLength GetWindowTextLengthA
HWND WINAPI GetParent(HWND);
HWND WINAPI SetParent(HWND, HWND);
HWND WINAPI GetAncestor(HWND, UINT);
#define GA_ROOT 2
HWND WINAPI GetDesktopWindow();
BOOL WINAPI IsWindow(HWND);
BOOL WINAPI IsWindowVisible(HWND);
BOOL WINAPI ShowWindow(HWND, int);
BOOL WINAPI EnableWindow(HWND, BOOL);
BOOL WINAPI GetWindowRect(HWND, LPRECT);
BOOL WINAPI GetClientRect(HWND, LPRECT);
BOOL WINAPI AdjustWindowRect(LPRECT, DWORD, BOOL);
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME)
BOOL WINAPI SetWindowPos(HWND, HWND, int, int, int, int, UINT);
BOOL WINAPI MoveWindow(HWND, int, int, int, int, BOOL);
BOOL WINAPI ClientToScreen(HWND, LPPOINT);
BOOL WINAPI ScreenToClient(HWND, LPPOINT);
int WINAPI MapWindowPoints(HWND, HWND, LPPOINT, UINT);
HWND WINAPI WindowFromPoint(POINT);
BOOL WINAPI InvalidateRect(HWND, const RECT*, BOOL);
BOOL WINAPI ValidateRect(HWND, const RECT*);
BOOL WINAPI InvalidateRgn(HWND, HRGN, BOOL);
BOOL WINAPI RedrawWindow(HWND, const RECT*, HRGN, UINT);
BOOL WINAPI UpdateWindow(HWND);
BOOL WINAPI GetUpdateRect(HWND, LPRECT, BOOL);
int WINAPI GetUpdateRgn(HWND, HRGN, BOOL);
HDC WINAPI BeginPaint(HWND, LPPAINTSTRUCT);
BOOL WINAPI EndPaint(HWND, const PAINTSTRUCT*);
BOOL WINAPI ScrollWindow(HWND, int, int, const RECT*, const RECT*);
int WINAPI SetWindowRgn(HWND, HRGN, BOOL);
BOOL WINAPI UpdateLayeredWindow(HWND, HDC, POINT*, SIZE*, HDC, POINT*, COLORREF, BLENDFUNCTION*, DWORD);
BOOL WINAPI SetLayeredWindowAttributes(HWND, COLORREF, BYTE, DWORD);
HWND WINAPI SetFocus(HWND);
HWND WINAPI GetFocus();
HWND WINAPI SetCapture(HWND);
HWND WINAPI GetCapture();
BOOL WINAPI ReleaseCapture();
HWND WINAPI GetActiveWindow();
HWND WINAPI GetForegroundWindow();
BOOL WINAPI SetForegroundWindow(HWND);
BOOL WINAPI TrackMouseEvent(LPTRACKMOUSEEVENT);
UINT_PTR WINAPI SetTimer(HWND, UINT_PTR, UINT, void*);
BOOL WINAPI KillTimer(HWND, UINT_PTR);

HCURSOR WINAPI LoadCursorA(HINSTANCE, LPCSTR);
#define LoadCursor LoadCursorA
HCURSOR WINAPI SetCursor(HCURSOR);
HCURSOR WINAPI GetCursor();
BOOL WINAPI GetCursorPos(LPPOINT);
BOOL WINAPI SetCursorPos(int, int);
int WINAPI ShowCursor(BOOL);
SHORT WINAPI GetAsyncKeyState(int);
SHORT WINAPI GetKeyState(int);
UINT WINAPI GetDoubleClickTime();
int WINAPI GetSystemMetrics(int);
DWORD WINAPI GetSysColor(int);
BOOL WINAPI MessageBeep(UINT);
int WINAPI MessageBoxA(HWND, LPCSTR, LPCSTR, UINT);
#define MessageBox MessageBoxA

HMENU WINAPI CreatePopupMenu();
HMENU WINAPI CreateMenu();
BOOL WINAPI DestroyMenu(HMENU);
BOOL WINAPI AppendMenuA(HMENU, UINT, UINT_PTR, LPCSTR);
#define AppendMenu AppendMenuA
BOOL WINAPI InsertMenuA(HMENU, UINT, UINT, UINT_PTR, LPCSTR);
#define InsertMenu InsertMenuA
BOOL WINAPI TrackPopupMenu(HMENU, UINT, int, int, int, HWND, const RECT*);
DWORD WINAPI CheckMenuItem(HMENU, UINT, UINT);
BOOL WINAPI EnableMenuItem(HMENU, UINT, UINT);
int WINAPI GetMenuItemCount(HMENU);

DWORD WINAPI GetTickCount();
void WINAPI Sleep(DWORD);
DWORD WINAPI GetLastError();
void WINAPI SetLastError(DWORD);
BOOL WINAPI GetVersionExA(LPOSVERSIONINFOA);
#define GetVersionEx GetVersionExA
HMODULE WINAPI LoadLibraryA(LPCSTR);
#define LoadLibrary LoadLibraryA
BOOL WINAPI FreeLibrary(HMODULE);
FARPROC WINAPI GetProcAddress(HMODULE, LPCSTR);
HMODULE WINAPI GetModuleHandleA(LPCSTR);
#define GetModuleHandle GetModuleHandleA
DWORD WINAPI GetModuleFileNameA(HMODULE, LPSTR, DWORD);
#define GetModuleFileName GetModuleFileNameA
int WINAPI MultiByteToWideChar(UINT, DWORD, LPCSTR, int, LPWSTR, int);
int WINAPI WideCharToMultiByte(UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPBOOL);
HGLOBAL WINAPI GlobalAlloc(UINT, size_t);
HGLOBAL WINAPI GlobalFree(HGLOBAL);
LPVOID WINAPI GlobalLock(HGLOBAL);
BOOL WINAPI GlobalUnlock(HGLOBAL);
size_t WINAPI GlobalSize(HGLOBAL);
UINT WINAPI DragQueryFileA(HDROP, UINT, LPSTR, UINT);
#define DragQueryFile DragQueryFileA
void WINAPI DragFinish(HDROP);
void WINAPI DragAcceptFiles(HWND, BOOL);
HRESULT WINAPI SHGetFolderPathA(HWND, int, HANDLE, DWORD, LPSTR);
#define SHGetFolderPath SHGetFolderPathA
BOOL WINAPI CreateDirectoryA(LPCSTR, void*);
#define CreateDirectory CreateDirectoryA
DWORD WINAPI GetFileAttributesA(LPCSTR);
#define GetFileAttributes GetFileAttributesA
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x10
BOOL WINAPI QueryPerformanceCounter(LONGLONG*);
BOOL WINAPI QueryPerformanceFrequency(LONGLONG*);
void WINAPI OutputDebugStringA(LPCSTR);
#define OutputDebugString OutputDebugStringA

#define STDMETHODCALLTYPE
#define STDMETHOD(m) virtual HRESULT STDMETHODCALLTYPE m
#define STDMETHOD_(t, m) virtual t STDMETHODCALLTYPE m
#define STDMETHODIMP HRESULT STDMETHODCALLTYPE
#define STDMETHODIMP_(t) t STDMETHODCALLTYPE
#define PURE = 0
#define E_NOINTERFACE ((HRESULT)0x80004002)
#define STGM_READ 0x00000000L
#define SLR_NO_UI 0x0001
#define SLR_ANY_MATCH 0x0002
#define SLGP_SHORTPATH 0x0001
typedef struct _POINTL { LONG x, y; } POINTL;
typedef WORD CLIPFORMAT;
typedef struct tagFORMATETC {
  CLIPFORMAT cfFormat;
  void* ptd;
  DWORD dwAspect;
  LONG lindex;
  DWORD tymed;
} FORMATETC;
struct IUnknown;
typedef struct tagSTGMEDIUM {
  DWORD tymed;
  HGLOBAL hGlobal;
  IUnknown* pUnkForRelease;
} STGMEDIUM;
inline bool operator==(const GUID& a, const GUID& b) { return memcmp(&a, &b, sizeof(GUID)) == 0; }
inline bool operator!=(const GUID& a, const GUID& b) { return !(a == b); }
struct IUnknown {
  virtual HRESULT QueryInterface(REFIID, void**) = 0;
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;
  virtual ~IUnknown() {}
};
typedef IUnknown* LPUNKNOWN;
struct IDataObject : IUnknown {
  virtual HRESULT GetData(FORMATETC*, STGMEDIUM*) = 0;
};
struct IDropTarget : IUnknown {
  virtual HRESULT DragEnter(IDataObject*, DWORD, POINTL, DWORD*) = 0;
  virtual HRESULT DragOver(DWORD, POINTL, DWORD*) = 0;
  virtual HRESULT DragLeave() = 0;
  virtual HRESULT Drop(IDataObject*, DWORD, POINTL, DWORD*) = 0;
};
struct IShellLinkA : IUnknown {
  virtual HRESULT GetPath(LPSTR, int, void*, DWORD) = 0;
  virtual HRESULT Resolve(HWND, DWORD) = 0;
};
typedef IShellLinkA IShellLink;
struct IPersistFile : IUnknown {
  virtual HRESULT Load(LPCWSTR, DWORD) = 0;
};
static const GUID IID_IUnknown = {0x00000000, 0x0000, 0x0000, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
static const GUID IID_IDropTarget = {0x00000122, 0x0000, 0x0000, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
static const GUID IID_IPersistFile = {0x0000010b, 0x0000, 0x0000, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
static const GUID IID_IShellLink = {0x000214EE, 0x0000, 0x0000, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
static const GUID CLSID_ShellLink = {0x00021401, 0x0000, 0x0000, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
void WINAPI ReleaseStgMedium(STGMEDIUM*);

HRESULT WINAPI OleInitialize(LPVOID);
void WINAPI OleUninitialize();
HRESULT WINAPI CoInitialize(LPVOID);
void WINAPI CoUninitialize();
HRESULT WINAPI CoCreateInstance(REFCLSID, void*, DWORD, REFIID, LPVOID*);
HRESULT WINAPI RegisterDragDrop(HWND, IDropTarget*);
HRESULT WINAPI RevokeDragDrop(HWND);

typedef struct _WIN32_FIND_DATAA {
  DWORD dwFileAttributes;
  DWORD ftCreationTime[2];
  DWORD ftLastAccessTime[2];
  DWORD ftLastWriteTime[2];
  DWORD nFileSizeHigh;
  DWORD nFileSizeLow;
  DWORD dwReserved0;
  DWORD dwReserved1;
  CHAR cFileName[MAX_PATH];
  CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA, WIN32_FIND_DATA, *LPWIN32_FIND_DATAA;
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define GENERIC_READ 0x80000000
#define GENERIC_WRITE 0x40000000
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define FILE_ATTRIBUTE_NORMAL 0x80
#define FILE_SHARE_READ 0x1
HANDLE WINAPI FindFirstFileA(LPCSTR, LPWIN32_FIND_DATAA);
#define FindFirstFile FindFirstFileA
BOOL WINAPI FindNextFileA(HANDLE, LPWIN32_FIND_DATAA);
#define FindNextFile FindNextFileA
BOOL WINAPI FindClose(HANDLE);
HANDLE WINAPI CreateFileA(LPCSTR, DWORD, DWORD, void*, DWORD, DWORD, HANDLE);
#define CreateFile CreateFileA
BOOL WINAPI WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, void*);
BOOL WINAPI ReadFile(HANDLE, LPVOID, DWORD, LPDWORD, void*);
BOOL WINAPI CloseHandle(HANDLE);
BOOL WINAPI DeleteFileA(LPCSTR);
#define DeleteFile DeleteFileA
BOOL WINAPI RemoveDirectoryA(LPCSTR);
#define RemoveDirectory RemoveDirectoryA
BOOL WINAPI MoveFileA(LPCSTR, LPCSTR);
#define MoveFile MoveFileA
DWORD WINAPI GetEnvironmentVariableA(LPCSTR, LPSTR, DWORD);
#define GetEnvironmentVariable GetEnvironmentVariableA
DWORD WINAPI GetTempPathA(DWORD, LPSTR);
#define GetTempPath GetTempPathA
BOOL WINAPI SetEnvironmentVariableA(LPCSTR, LPCSTR);
#define SetEnvironmentVariable SetEnvironmentVariableA
int _putenv_s(const char*, const char*);
#define GW_HWNDFIRST 0
#define GW_HWNDLAST 1
#define GW_HWNDNEXT 2
#define GW_HWNDPREV 3
#define GW_OWNER 4
#define GW_CHILD 5
HWND WINAPI GetWindow(HWND, UINT);
HWND WINAPI FindWindowExA(HWND, HWND, LPCSTR, LPCSTR);
#define FindWindowEx FindWindowExA
int WINAPI GetWindowRgn(HWND, HRGN);
DWORD WINAPI GetCurrentProcessId();
DWORD WINAPI GetCurrentThreadId();

#define TA_LEFT 0
#define TA_RIGHT 2
#define TA_CENTER 6
#define TA_TOP 0
#define TA_BOTTOM 8
#define TA_BASELINE 24
#define TA_UPDATECP 1
#define ETO_OPAQUE 0x0002
#define ETO_CLIPPED 0x0004
#define COLOR_MENU 4
#define COLOR_MENUTEXT 7
#define COLOR_WINDOWFRAME 6
#define COLOR_3DSHADOW 16
#define COLOR_3DHILIGHT 20
#define COLOR_3DDKSHADOW 21
#define COLOR_3DLIGHT 22
#define COLOR_INFOTEXT 23
#define COLOR_INFOBK 24

FILE* softwin_fopen(const char* path, const char* mode);
int _vsnprintf_s(char*, size_t, size_t, const char*, va_list);
int _snprintf_s(char*, size_t, size_t, const char*, ...);
int sprintf_s(char*, size_t, const char*, ...);
int strcpy_s(char*, size_t, const char*);
int strncpy_s(char*, size_t, const char*, size_t);
int strcat_s(char*, size_t, const char*);
int _stricmp(const char*, const char*);
int _strnicmp(const char*, const char*, size_t);
#define stricmp _stricmp
#define strnicmp _strnicmp
#define _snprintf snprintf
#define _vsnprintf vsnprintf

}

#include "softwin_api.h"

namespace std {
using ::softwin_fopen;
}
#define fopen(p, m) softwin_fopen(p, m)
