/* xinu_includes.h - Wrapper for XINU code compilation.
 * Generated on: 2025-06-18 01:39:50 by mod
 */
#ifndef _XINU_INCLUDES_H_ 
#define _XINU_INCLUDES_H_

#define _CRT_SECURE_NO_WARNINGS 
#define XINU_SIMULATION        

/* These must be defined before XINU inclusion */
typedef int devcall;
typedef int syscall;
typedef unsigned char byte;

/* --- Function Redirection Shims --- */
#ifdef getchar 
#undef getchar
#endif
#define getchar() getchar()

#ifdef putchar
#undef putchar
#endif
#define putchar(c) putchar(c)

/* Prevent include conflicts by removing XINU's overrides */
#ifdef scanf
#undef scanf
#endif

#ifdef sscanf
#undef sscanf
#endif

#ifdef fscanf
#undef fscanf
#endif

#ifdef printf
#undef printf
#endif

#ifdef sprintf
#undef sprintf
#endif

#ifdef fprintf
#undef fprintf
#endif

/* Include XINU headers */
#include "xinu.h" 

#endif /* _XINU_INCLUDES_H_ */
