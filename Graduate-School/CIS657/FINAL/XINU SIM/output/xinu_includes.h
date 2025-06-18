/* xinu_includes.h - Wrapper for XINU code compilation.
 * Generated on: 2025-06-18 13:27:26 by mol
 */
#ifndef _XINU_INCLUDES_H_ 
#define _XINU_INCLUDES_H_

#define _CRT_SECURE_NO_WARNINGS 
#define XINU_SIMULATION        

/* Windows compatibility - Minimal version */
#ifdef _WIN32
  /* Include the Windows compatibility header */
  #include "xinu_windows_compat.h"
#endif

/* Include our minimal definitions */
#include "xinu_stddefs.h" 

#endif /* _XINU_INCLUDES_H_ */
