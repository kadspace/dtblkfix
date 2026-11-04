#ifndef _PNG_VST_GUI_H_
#define _PNG_VST_GUI_H_

#include "VstGuiSupport.h"
#include <ostream>

// load a PNG file into a CContextRGBA, error & warning messages are written to "err"
// DtBlkFix: scaled to "scale" pixels per view unit, see CContextRGBA::create
bool /*success*/ ReadPng(CContextRGBA* dst, string file_name, ostream* err_str, float scale = 1.0f);

#endif
