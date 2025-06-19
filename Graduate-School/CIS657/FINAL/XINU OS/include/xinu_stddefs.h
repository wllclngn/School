/* xinu_stddefs.h - Extended type definitions to prevent circular dependencies */
/* Generated on: 2025-06-18 20:27:34 */
/* By user: mod */
#ifndef _XINU_STDDEFS_H_
#define _XINU_STDDEFS_H_

/* Version information */
#define VERSION "XINU Simulation Version 1.0"

/* Basic type definitions to prevent circular dependencies */
typedef char                int8;
typedef short               int16;
typedef int                 int32;
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

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
