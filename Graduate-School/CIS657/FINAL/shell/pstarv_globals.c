/**
 * @file pstarv_globals.c
 * Global variables for process starvation prevention
 */

#include <xinu.h>

/* Starvation fix global variables */
bool8 enable_starvation_fix = FALSE;    /* FALSE = disabled, TRUE = enabled */
pid32 pstarv_pid = BADPID;              /* PID of process to monitor for starvation */
uint32 pstarv_ready_time = 0;           /* Time when pstarv entered ready queue */
uint32 last_boost_time = 0;             /* Last time priority was boosted */