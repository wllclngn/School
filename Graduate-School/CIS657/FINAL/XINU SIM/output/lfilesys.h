/* lfilesys.h - Generated stub header for XINU OS simulation
 * Generated on: 2025-06-18 22:06:19
 */
#ifndef _LFILESYS_H_
#define _LFILESYS_H_

#include "base_types.h"

/* Minimal stub for filesystem module */
#include "file.h"             /* Include file header for types */

#define FNAMLEN     16          /* Length of filename */

/* Minimal filesystem structure stubs */
typedef struct lfdir {
    int dummy;
} lfdir_t;

typedef struct lflcblk {
    int dummy;
} lflcblk_t;

#endif /* _LFILESYS_H_ */
