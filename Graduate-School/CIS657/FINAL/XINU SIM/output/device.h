/* device.h - Generated stub header for XINU OS simulation
 * Generated on: 2025-06-18 22:15:49
 */
#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "base_types.h"

/* Device definitions stub */
#define DEVNAMLEN   16      /* Length of device name      */

/* Device table declarations */
typedef struct dentry {
    int  dvnum;              /* Device number */
    char dvname[DEVNAMLEN];  /* Device name */
    int   (*dvputc)(int, char); /* Function pointer for putting chars */
    int   (*dvgetc)(int);       /* Function pointer for getting chars */
} dentry_t;

extern dentry_t devtab[];     /* Device table */

#endif /* _DEVICE_H_ */
