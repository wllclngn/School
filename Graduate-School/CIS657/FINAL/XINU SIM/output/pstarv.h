/* pstarv.h - Generated stub header for XINU OS simulation
 * Generated on: 2025-06-18 22:15:49
 */
#ifndef _PSTARV_H_
#define _PSTARV_H_

#include "base_types.h"

/* Starvation prevention definitions */
extern int32 starvingPID;       /* PID to monitor for starvation */
extern int32 starvation_prevention; /* Starvation prevention flag */
extern int32 pstarv_pid;        /* Process to monitor for Q2 */
extern int32 enable_starvation_fix; /* Enable starvation fix */
extern int32 pstarv_ready_time; /* Time when pstarv entered ready queue */
extern int32 last_boost_time;   /* Last priority boost time */

void boost_pstarv_priority(void);
void check_pstarv_time(void);
int32 updatepriostarv(int32 pid, int16 newprio);

#endif /* _PSTARV_H_ */
