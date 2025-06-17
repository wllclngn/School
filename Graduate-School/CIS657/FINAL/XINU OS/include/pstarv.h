#include <xinu.h>
/* pstarv.h - Header file for process starvation prevention */

#ifndef _PSTARV_H_
#define _PSTARV_H_

/* External declarations */
extern pid32 pstarv_pid;        /* PID of the process that might suffer starvation */
extern bool8 enable_starvation_fix; /* Flag to enable/disable the starvation fix */

/* Function prototypes */
syscall starvation_q1_init(void);  /* Initialize Q1 starvation prevention */
syscall starvation_q2_init(void);  /* Initialize Q2 starvation prevention */

/* Process function prototypes */
process process_p1(void);      /* P1 - High priority process */
process process_p2(void);      /* P2 - High priority process */
process process_pstarv(void);  /* Pstarv - Lower priority process that might starve */

/* Test function prototypes */
process run_starvation_test_q1(void);  /* Run Q1 test (context switch based) */
process run_starvation_test_q2(void);  /* Run Q2 test (time based) */

#endif /* _PSTARV_H_ */
