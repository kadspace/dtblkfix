#include <StdAfx.h>

#include "../libpng/png.h"
#include "PngVstGui.h"

using namespace std;

//-------------------------------------------------------------------------------------------------
struct InternalReadPng
// internal glue
{
  string file_name;
  _Ptr<png_struct> png_ptr;
  _Ptr<png_info> info_ptr;
  _Ptr<FILE> fp;

  // errors & other messages are written here
  ostream* err_str;

  // whether reading was ok
  bool ok;

  InternalReadPng(CContextRGBA* dst, string file_name_, ostream* err_str_);
  ~InternalReadPng();
};

//-------------------------------------------------------------------------------------------------
static void PngErrorCb(png_structp png_ptr, png_const_charp err_msg)
{
  InternalReadPng* t = (InternalReadPng*)(png_ptr->error_ptr);
  if (t->err_str)
    *t->err_str << "PNG: " << err_msg << " while reading " << t->file_name << endl;
}

//-------------------------------------------------------------------------------------------------
static void PngWarningCb(png_structp png_ptr, png_const_charp warn_msg)
{
  InternalReadPng* t = (InternalReadPng*)(png_ptr->error_ptr);
  if (t->err_str)
    *t->err_str << "PNG warning: " << warn_msg << " while reading " << t->file_name << endl;
}

//-------------------------------------------------------------------------------------------------
InternalReadPng::~InternalReadPng()
{
  /* clean up after the read, and free any memory allocated - REQUIRED */
  if (png_ptr)
    png_destroy_read_struct(&png_ptr.ptr, info_ptr ? &info_ptr.ptr : NULL, NULL);

  if (fp)
    fclose(fp);
}

//-------------------------------------------------------------------------------------------------
InternalReadPng::InternalReadPng(CContextRGBA* dst, string file_name_, ostream* err_str_)
{
  ok = false;
  file_name = file_name_;
  err_str = err_str_;

  if (!dst) {
    if (err_str)
      *err_str << "PNG: Internal error, no memory for destination" << file_name << endl;
    return;
  }

  fp = fopen(file_name.c_str(), "rb");
  if (!fp) {
    if (err_str)
      *err_str << "PNG: Couldn't open " << file_name << endl;
    return;
  }

  //
  png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)this, &PngErrorCb, &PngWarningCb);

  if (!png_ptr) {
    if (err_str)
      *err_str << "PNG: internal error, couldn't create read structure while reading " << file_name
               << endl;
    return;
  }

  //
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    if (err_str)
      *err_str << "PNG: internal error, couldn't create info structure while reading " << file_name
               << endl;
    return;
  }

  // if there's an error, we jump back to here
  if (setjmp(png_jmpbuf(png_ptr))) {
    if (err_str)
      *err_str << "PNG: error while reading " << file_name << endl;
    return;
  }

  //
  png_init_io(png_ptr, fp);

  png_read_png(png_ptr,
               info_ptr,
               PNG_TRANSFORM_SWAP_ENDIAN |  // swap bytes if 16 bit
                   PNG_TRANSFORM_STRIP_16 | // turn 16 bit into 8 bit
                   PNG_TRANSFORM_PACKING,   // expand 1,2 or 4 to 8 bit
               NULL);

  // see whether this file is good
  png_uint_32 width;
  png_uint_32 height;
  int bit_depth;
  int color_type;
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);

  if (bit_depth != 8 || !(color_type & PNG_COLOR_TYPE_RGB)) {
    if (err_str)
      *err_str << "PNG: format of " << file_name << " is not PNG24 or PNG32" << endl;
    return;
  }

  // create the bitmap
  dst->create(width, height);
  if (!dst->ok()) {
    if (err_str)
      *err_str << "PNG: couldn't create bitmap while reading " << file_name << ", width=" << width
               << ", height=" << height << endl;
    return;
  }
  bool has_alpha = color_type & PNG_COLOR_MASK_ALPHA ? true : false;

  // copy data to our own bitmap
  png_bytepp src_rows = png_get_rows(png_ptr, info_ptr);
  for (png_uint_32 y = 0; y < height; y++) {

    // process a line
    png_bytep src_p = src_rows[y];
    if (has_alpha) {
      // PNG 32
      for (RowRGBA dst_p = dst->getRow(y); !dst_p.isEnd(); dst_p.incX()) {
        unsigned r = *src_p++, g = *src_p++, b = *src_p++, a = *src_p++;
        dst_p.set(r, g, b, a);
      }
    }
    else {
      // PNG 24
      for (RowRGBA dst_p = dst->getRow(y); !dst_p.isEnd(); dst_p.incX()) {
        unsigned r = *src_p++, g = *src_p++, b = *src_p++;
        dst_p.set(r, g, b);
      }
    }
  }

  //
  ok = true;
}

//-------------------------------------------------------------------------------------------------
// DtBlkFix: image scaling for the UI scale
//-------------------------------------------------------------------------------------------------
struct ResampleTaps {
  int first;             // first source pixel
  std::vector<float> wt; // weights, sum to 1
};

//-------------------------------------------------------------------------------------------------
static float CatmullRom(float x)
// cubic convolution kernel with a = -0.5
{
  x = fabsf(x);
  if (x < 1.0f)
    return (1.5f * x - 2.5f) * x * x + 1.0f;
  if (x < 2.0f)
    return ((-0.5f * x + 2.5f) * x - 4.0f) * x + 2.0f;
  return 0.0f;
}

//-------------------------------------------------------------------------------------------------
static void ResampleTapsFor(int src_n, int dst_n, std::vector<ResampleTaps>* taps)
// filter taps for each destination pixel, the filter is widened when shrinking
{
  float scale = (float)dst_n / (float)src_n;
  float stretch = scale < 1.0f ? 1.0f / scale : 1.0f;
  float support = 2.0f * stretch;
  taps->resize(dst_n);
  for (int i = 0; i < dst_n; i++) {
    float centre = ((float)i + 0.5f) / scale - 0.5f;
    int first = (int)floorf(centre - support) + 1;
    int last = (int)floorf(centre + support);
    ResampleTaps& t = (*taps)[i];
    t.first = first;
    t.wt.resize(last - first + 1);
    float sum = 0.0f;
    for (int j = first; j <= last; j++)
      sum += t.wt[j - first] = CatmullRom(((float)j - centre) / stretch);
    for (int j = 0; j < (int)t.wt.size(); j++)
      t.wt[j] /= sum;
  }
}

//-------------------------------------------------------------------------------------------------
static void ResampleImage(CContextRGBA* src, CContextRGBA* dst)
// resample all of src into all of dst (premultiplied alpha)
{
  int sw = src->getPixelWidth(), sh = src->getPixelHeight();
  int dw = dst->getPixelWidth(), dh = dst->getPixelHeight();
  std::vector<ResampleTaps> tx, ty;
  ResampleTapsFor(sw, dw, &tx);
  ResampleTapsFor(sh, dh, &ty);

  // horizontal pass into floats
  std::vector<float> rows((size_t)sh * dw * 4);
  for (int y = 0; y < sh; y++) {
    for (int x = 0; x < dw; x++) {
      float acc[4] = {0, 0, 0, 0};
      const ResampleTaps& t = tx[x];
      for (int j = 0; j < (int)t.wt.size(); j++) {
        int sx = limit_range(t.first + j, 0, sw - 1);
        PtrRGBA p = src->data(sx, y);
        acc[0] += t.wt[j] * p.getRed();
        acc[1] += t.wt[j] * p.getGreen();
        acc[2] += t.wt[j] * p.getBlue();
        acc[3] += t.wt[j] * p.getAlpha();
      }
      for (int c = 0; c < 4; c++)
        rows[((size_t)y * dw + x) * 4 + c] = acc[c];
    }
  }

  // vertical pass into dst
  for (int y = 0; y < dh; y++) {
    const ResampleTaps& t = ty[y];
    for (int x = 0; x < dw; x++) {
      float acc[4] = {0, 0, 0, 0};
      for (int j = 0; j < (int)t.wt.size(); j++) {
        int sy = limit_range(t.first + j, 0, sh - 1);
        const float* p = &rows[((size_t)sy * dw + x) * 4];
        for (int c = 0; c < 4; c++)
          acc[c] += t.wt[j] * p[c];
      }
      unsigned long a = (unsigned long)limit_range(acc[3] + 0.5f, 0.0f, 255.0f);
      unsigned long r = (unsigned long)limit_range(acc[0] + 0.5f, 0.0f, (float)a);
      unsigned long g = (unsigned long)limit_range(acc[1] + 0.5f, 0.0f, (float)a);
      unsigned long b = (unsigned long)limit_range(acc[2] + 0.5f, 0.0f, (float)a);
      dst->data(x, y)._set(r, g, b, a); // already premultiplied
    }
  }
}

//-------------------------------------------------------------------------------------------------
bool /*success*/ ReadPng(CContextRGBA* dst, string file_name, ostream* err_str, float scale)
// outside interface, read the PNG file into the alpha context
// DtBlkFix: scale it to "scale" pixels per view unit (dst keeps the original size in view units)
{
  if (scale == 1.0f) {
    InternalReadPng read_png(dst, file_name, err_str);
    return read_png.ok;
  }

  CContextRGBA orig;
  {
    InternalReadPng read_png(&orig, file_name, err_str);
    if (!read_png.ok)
      return false;
  }
  dst->create(orig.getWidth(), orig.getHeight(), scale);
  if (!dst->ok())
    return false;
  ResampleImage(&orig, dst);
  return true;
}
