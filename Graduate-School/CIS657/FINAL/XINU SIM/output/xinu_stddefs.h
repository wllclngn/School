/* xinu_stddefs.h - Minimal type definitions for XINU simulation */
/* Generated on: 2025-06-19 00:07:27 */
/* By user: mol */
#ifndef _XINU_STDDEFS_H_
#define _XINU_STDDEFS_H_

/* Include base types */
#include "base_types.h"

/* Version information */
#define VERSION "XINU Simulation Version 1.0"

#ifdef _WIN32
/* Windows-specific compatibility */

/* Basic constants that don't conflict */
#ifndef NULLCH
#define NULLCH '\0'
#endif

#else
/* Non-Windows platforms - Standard definitions */
typedef unsigned char byte;
#endif /* _WIN32 */

#endif /* _XINU_STDDEFS_H_ */
