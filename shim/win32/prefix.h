#ifndef _DTBLKFIX_WIN32_PREFIX_H_
#define _DTBLKFIX_WIN32_PREFIX_H_
/**************************************************************************************************
DtBlkFix Win32 shim: prefix header (see windows.h in this folder)

This program is free software; you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

***************************************************************************************************/

#ifdef __cplusplus

#ifndef _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR
#  define _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR 1
#endif

#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <cmath>
#include <complex>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <typeinfo>
#include <utility>
#include <valarray>
#include <vector>

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <xmmintrin.h>

#include <fftw3.h>
#include <png.h>

#include "windows.h"

#ifndef _WIN32
#  define DTBLKFIX_WIN32_SHIM_LONG32 1
#  define long int
#  define DTBLKFIX_INT64 int64_t
#  undef LONG_MIN
#  undef LONG_MAX
#  undef ULONG_MAX
#  define LONG_MIN INT_MIN
#  define LONG_MAX INT_MAX
#  define ULONG_MAX UINT_MAX
#  define fopen dtblkfix_win32_fopen
#  define _WIN32 1
#  define WIN32 1
#  define WINDOWS 1
#  define _WINDOWS 1
#endif

#endif
#endif
