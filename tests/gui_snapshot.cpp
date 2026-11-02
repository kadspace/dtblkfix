// DtBlkFix tests: open the editor in a window, play audio through it and save a screenshot.
//
// dtblkfix_gui_snapshot <folder holding dtblkfx\> <out.png> [--scale s] [--skin folder]
//                       [--preset n]
//
// For looking at UI scaling and skins (CI uploads the screenshots).

#include <StdAfx.h>

#include "../libpng/png.h"
#include "DtBlkFx.hpp"
#include "Gui.h"
#include "harness.h"

extern CharArray<4096> g_plugin_path;
extern void* hInstance;
bool DtBlkFixGlobalInit();
bool DtBlkFixLoadImages(const char* skin_dir, float scale, GuiImages* dst, ostream* err);

// show the Size & Skin menus as the plugin does
struct NoViewChange : public GuiViewListener {
  virtual void guiViewChange(int, int) {}
};

static bool WritePng(const char* file, int w, int h, const std::vector<unsigned char>& rgb)
{
  FILE* f = fopen(file, "wb");
  if (!f)
    return false;
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png_create_info_struct(png);
  if (setjmp(png_jmpbuf(png))) {
    fclose(f);
    return false;
  }
  png_init_io(png, f);
  png_set_IHDR(png,
               info,
               w,
               h,
               8,
               PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);
  for (int y = 0; y < h; y++)
    png_write_row(png, (png_bytep)&rgb[(size_t)y * w * 3]);
  png_write_end(png, info);
  png_destroy_write_struct(&png, &info);
  fclose(f);
  return true;
}

static void Pump()
{
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    fprintf(
        stderr, "usage: %s <dir> <out.png> [--scale s] [--skin folder] [--preset n]\n", argv[0]);
    return 2;
  }
  float scale = 1.0f;
  const char* skin = NULL;
  int preset = 33;
  int hold_ms = 0; // keep the editor live this long before the screenshot
  for (int i = 3; i + 1 < argc; i += 2) {
    if (!strcmp(argv[i], "--scale"))
      scale = (float)atof(argv[i + 1]);
    else if (!strcmp(argv[i], "--skin"))
      skin = argv[i + 1];
    else if (!strcmp(argv[i], "--preset"))
      preset = atoi(argv[i + 1]);
    else if (!strcmp(argv[i], "--hold"))
      hold_ms = atoi(argv[i + 1]) * 1000;
  }

  // pixels are pixels (StdAfx.h targets Windows 2000, so look this up)
  BOOL(WINAPI * set_dpi_aware)() = NULL;
  GetProcAddress(set_dpi_aware, GetModuleHandleA("user32.dll"), "SetProcessDPIAware");
  if (set_dpi_aware)
    set_dpi_aware();
  hInstance = GetModuleHandle(NULL);
  g_plugin_path << argv[1] << "\\";
  if (!DtBlkFixGlobalInit())
    return 1;

  DtBlkFx* fx = new DtBlkFx(NULL);
  Gui* gui = fx->gui();
  if (!gui) {
    fprintf(stderr, "no editor (images missing?)\n");
    return 1;
  }
  fx->setSampleRate(44100.0f);
  fx->setBlockSize(512);
  fx->resume();
  fx->setProgram(preset);

  NoViewChange listener;
  GuiImages images = gui->_images;
  std::string skin_dir = std::string(argv[1]) + "\\dtblkfx";
  if (skin)
    skin_dir = skin;
  if (!DtBlkFixLoadImages(skin_dir.c_str(), scale, &images, &std::cerr))
    return 1;
  std::vector<std::string> skins(1, "Original");
  int scales[] = {75, 100, 125, 150, 200};
  gui->setView(scale, images, skins, 0, std::vector<int>(scales, scales + 5), &listener);

  // a window to hold the editor
  RECT r = {0, 0, gui->getScaledWidth(), gui->getScaledHeight()};
  AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
  HWND wnd = CreateWindowA("STATIC",
                           "DtBlkFix",
                           WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                           10,
                           10,
                           r.right - r.left,
                           r.bottom - r.top,
                           NULL,
                           NULL,
                           NULL,
                           NULL);
  ShowWindow(wnd, SW_SHOW);
  if (!gui->open(wnd)) {
    fprintf(stderr, "couldn't open the editor\n");
    return 1;
  }

  // 3 seconds of audio, idling the editor as a host would
  harness::Signal sig;
  std::vector<float> in_l(512), in_r(512), out_l(512), out_r(512);
  for (int b = 0; b < 3 * 44100 / 512; b++) {
    for (int i = 0; i < 512; i++)
      sig.next(in_l[i], in_r[i]);
    float* in[2] = {&in_l[0], &in_r[0]};
    float* out[2] = {&out_l[0], &out_r[0]};
    fx->processReplacing(in, out, 512);
    gui->idle();
    Pump();
    Sleep(5);
  }
  InvalidateRect(wnd, NULL, TRUE);
  for (int i = 0; i < 20 + hold_ms / 10; i++) {
    gui->idle();
    Pump();
    Sleep(10);
  }

  // grab the client area off the screen
  POINT org = {0, 0};
  ClientToScreen(wnd, &org);
  int w = gui->getScaledWidth(), h = gui->getScaledHeight();
  HDC screen = GetDC(NULL);
  HDC mem = CreateCompatibleDC(screen);
  BITMAPINFO bi;
  memset(&bi, 0, sizeof(bi));
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = w;
  bi.bmiHeader.biHeight = -h;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  void* bits = NULL;
  HBITMAP bm = CreateDIBSection(mem, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
  SelectObject(mem, bm);
  BitBlt(mem, 0, 0, w, h, screen, org.x, org.y, SRCCOPY);
  GdiFlush();
  std::vector<unsigned char> rgb((size_t)w * h * 3);
  for (int i = 0; i < w * h; i++) {
    unsigned long p = ((unsigned long*)bits)[i];
    rgb[i * 3 + 0] = (unsigned char)(p >> 16);
    rgb[i * 3 + 1] = (unsigned char)(p >> 8);
    rgb[i * 3 + 2] = (unsigned char)p;
  }
  bool ok = WritePng(argv[2], w, h, rgb);
  printf("%s: %dx%d at %d%%\n", argv[2], w, h, RndToInt(scale * 100.0f));

  gui->close();
  DestroyWindow(wnd);
  delete fx;
  return ok ? 0 : 1;
}
