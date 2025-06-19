/* process.h - Generated stub header for XINU OS simulation
 * Generated on: 2025-06-18 21:59:50
 */
#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "base_types.h"

/* Process type definitions */
typedef int32 pid32;            /* Process ID type */
typedef int16 pri16;            /* Process priority */

/* Process state constants */
#define PR_FREE      0          /* Process table entry is unused */
#define PR_CURR      1          /* Process is running */
#define PR_READY     2          /* Process is on ready queue */
#define PR_SLEEP     4          /* Process is sleeping */
#define PR_SUSP      5          /* Process is suspended */

#endif /* _PROCESS_H_ */
