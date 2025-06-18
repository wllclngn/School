/* xinu_windows_compat.h - Compatibility layer for XINU on Windows
 * Generated on: 2025-06-18 14:23:09 by mol
 * AUTOMATICALLY GENERATED - DO NOT EDIT DIRECTLY
 */
#ifndef _XINU_WINDOWS_COMPAT_H_
#define _XINU_WINDOWS_COMPAT_H_

/* This compatibility header provides Windows-specific functions
 * without conflicting with XINU types.
 */

#ifdef _WIN32

/* Forward-declare needed types without redefining XINU's types */
struct dentry;  /* XINU device entry structure */

/* Forward-declare Windows compatibility functions */
int xinu_putc(int dev, char c);
int xinu_getc(int dev);
int xinu_open(int dev, char *name, char *mode);
int xinu_close(int dev);

/* Remap commonly conflicting functions */
#define putc xinu_putc
#define getc xinu_getc
#define open xinu_open
#define close xinu_close

/* Forward declarations for common Windows structs - NO WINDOWS.H */
struct HWND__;
struct HDC__;
typedef struct HWND__ *HWND;
typedef struct HDC__ *HDC;
typedef unsigned long DWORD;
typedef int BOOL;

#endif /* _WIN32 */

#endif /* _XINU_WINDOWS_COMPAT_H_ */
