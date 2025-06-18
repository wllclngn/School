/* pstarv.h - Modified for XINU Final Project
 * Last modified: 2025-06-15 05:35:28 UTC
 * Modified by: wllclngn
 */

#ifndef _PSTARV_H_
#define _PSTARV_H_

#include <xinu.h>

/* Global variable declarations */
extern pid32 pstarv_pid;              /* PID of process to monitor for starvation */
extern bool8 enable_starvation_fix;   /* Starvation prevention flag */
extern uint32 pstarv_ready_time;      /* Time when pstarv entered ready queue */
extern uint32 last_boost_time;        /* Last time priority was boosted */

/* Function prototypes */
extern void boost_pstarv_priority(void);

#endif /* _PSTARV_H_ */