/**
 * @file pstarv_globals.c
 * Global variables for process starvation prevention
 */

#include <xinu.h>

/* Starvation fix global variables */
int enable_starvation_fix = 0;      /* 0 = disabled, 1 = enabled */
int pstarv_pid = -1;                /* PID of process to monitor for starvation */
uint32 pstarv_ready_time = 0;       /* Time when pstarv entered ready queue */
uint32 last_boost_time = 0;         /* Last time priority was boosted */