/* pstarv_globals.c - Global variables for starvation prevention */

#include <xinu.h>
#include <pstarv.h>

/* Global variables for starvation prevention */
pid32 starvingPID = BADPID;         /* ID of process that might suffer starvation (Q1) */
bool8 starvation_prevention = FALSE; /* Starvation prevention flag for Q1 */
pid32 pstarv_pid = BADPID;          /* PID of process to monitor for starvation (Q2) */
bool8 enable_starvation_fix = FALSE; /* Starvation prevention flag for Q2 */
uint32 pstarv_ready_time = 0;       /* Time when pstarv entered ready queue */
uint32 last_boost_time = 0;         /* Last time priority was boosted */
