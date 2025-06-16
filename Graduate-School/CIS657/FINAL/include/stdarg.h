/**
 * @file stdarg.h
 * @provides va_copy, va_start, va_arg, va_end.
 *
 * $Id: stdarg.h 2020 2009-08-13 17:50:08Z mschul $
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

/* Conditional compilation for GCC-specific varargs:
 * Only define these if NOT compiling for Windows/MSVC simulation
 * OR if explicitly using GCC (even in a simulation, though less common).
 * For MSVC simulation, its own <stdarg.h> (already included via xinu_includes.h)
 * provides compatible definitions.
 */
#if (defined(__GNUC__) && !defined(_MSC_VER)) || (!defined(XINU_SIMULATION) && !defined(_MSC_VER))

/* GCC-specific varargs */
typedef __builtin_va_list va_list;

#define va_copy(dst, src)	__builtin_va_copy(dst, src)
#define va_start(last, va)	__builtin_va_start(last, va)
#define va_arg(va, type)	__builtin_va_arg(va, type)
#define va_end(va)		__builtin_va_end(va)

#endif /* (__GNUC__ && !_MSC_VER) || (!XINU_SIMULATION && !_MSC_VER) */