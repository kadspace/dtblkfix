#ifndef _DTBLKFIX_WIN32_XMMINTRIN_H_
#define _DTBLKFIX_WIN32_XMMINTRIN_H_
/**************************************************************************************************
DtBlkFix Win32 shim: the control register calls the engine makes through <xmmintrin.h>, for
processors that have no SSE (see ../windows.h)

This program is free software; you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

***************************************************************************************************/

#if defined(__x86_64__) || defined(__i386__)
#  include_next <xmmintrin.h>
#else

#  define _MM_MASK_INVALID 0x0080
#  define _MM_MASK_DENORM 0x0100
#  define _MM_MASK_DIV_ZERO 0x0200
#  define _MM_MASK_OVERFLOW 0x0400
#  define _MM_MASK_UNDERFLOW 0x0800
#  define _MM_MASK_INEXACT 0x1000
#  define _MM_MASK_MASK 0x1f80
#  define _MM_FLUSH_ZERO_ON 0x8000
#  define _MM_FLUSH_ZERO_OFF 0x0000
#  define _MM_FLUSH_ZERO_MASK 0x8000

#  if defined(__aarch64__)
static inline unsigned int _mm_getcsr(void)
{
  unsigned long long fpcr = __builtin_arm_rsr64("FPCR");
  return _MM_MASK_MASK | ((fpcr & (1ull << 24)) ? _MM_FLUSH_ZERO_ON : 0);
}
static inline void _mm_setcsr(unsigned int csr)
{
  unsigned long long fpcr = __builtin_arm_rsr64("FPCR");
  if (csr & _MM_FLUSH_ZERO_ON)
    fpcr |= 1ull << 24;
  else
    fpcr &= ~(1ull << 24);
  __builtin_arm_wsr64("FPCR", fpcr);
}
#  else
static inline unsigned int _mm_getcsr(void)
{
  return _MM_MASK_MASK;
}
static inline void _mm_setcsr(unsigned int csr)
{
  (void)csr;
}
#  endif

#endif
#endif
