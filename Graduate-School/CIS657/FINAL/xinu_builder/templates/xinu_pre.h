/* XINU Pre-include compatibility header - This file is included before any other header */
#ifndef _XINU_PRE_INCLUDE_H_
#define _XINU_PRE_INCLUDE_H_

/* Only define types if they aren't already defined */
#ifndef _XINU_TYPEDEFS_DEFINED
#define _XINU_TYPEDEFS_DEFINED
typedef int devcall;
/* Don't redefine int8 if it's already defined */
#ifndef _INT8_DEFINED_
#define _INT8_DEFINED_
typedef signed char int8;
#endif
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;
#endif

/* Define basic XINU constants */
#ifndef NULL
#define NULL 0
#endif

#ifndef OK
#define OK 1
#endif

#ifndef SYSERR
#define SYSERR (-1)
#endif

#ifndef EOF
#define EOF (-2)
#endif

#ifndef BADPID
#define BADPID (-1)
#endif

#ifndef SHELL_OK
#define SHELL_OK 1
#endif

#ifndef SHELL_ERROR
#define SHELL_ERROR (-1)
#endif

/* Forward declare structures rather than defining them */
struct dentry;
struct uart_csreg;
struct ttycblk;

/* Fix for conflicting function names */
#define bzero(x, n) memset((x), 0, (n))

#endif /* _XINU_PRE_INCLUDE_H_ */
