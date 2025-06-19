/* pstarv.h - Definitions for Starvation Prevention */

#ifndef _PSTARV_H_
#define _PSTARV_H_

/* Include only what's needed, not the entire xinu.h */
#include <kernel.h>
#include <conf.h>
#include <process.h>

/* Global variable declarations */
extern pid32 starvingPID;            /* PID of process to monitor for starvation (Q1) */
extern bool8 starvation_prevention;  /* Starvation prevention flag for Q1 */
extern pid32 pstarv_pid;             /* PID of process to monitor for starvation (Q2) */
extern bool8 enable_starvation_fix;  /* Starvation prevention flag for Q2 */
extern uint32 pstarv_ready_time;     /* Time when pstarv entered ready queue */
extern uint32 last_boost_time;       /* Last time priority was boosted */

/* Function prototypes */
extern void boost_pstarv_priority(void);
extern void check_pstarv_time(void);
extern syscall updatepriostarv(pid32 pid, pri16 newprio);

#endif /* _PSTARV_H_ */
