/* rdisksys.h - Generated stub header for XINU OS simulation
 * Generated on: 2025-06-18 21:59:50
 */
#ifndef _RDISKSYS_H_
#define _RDISKSYS_H_

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

#endif /* _RDISKSYS_H_ */
