/* starvation_shell.c - Modified for XINU Final Project
 * Last modified: 2025-06-16 00:35:12 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

extern void p1_func(void);
extern void p2_func(void);
extern void pstarv_func(void);

// New function for PStarv to check its ready time and boost priority
void check_pstarv_time(void) {
    uint32 current_time;
    uint32 time_in_ready_queue;

    // Make sure this only runs for Q2 (time-based prevention)
    if (enable_starvation_fix == FALSE && pstarv_pid != BADPID) {
        struct procent *pstarv = &proctab[pstarv_pid];
        
        if (pstarv->prstate == PR_READY) {
            current_time = clktime;
            
            // If we've not set a ready time yet, set it now
            if (pstarv_ready_time == 0) {
                pstarv_ready_time = current_time;
                kprintf("Initializing pstarv_ready_time to %d\n", pstarv_ready_time);
            }
            
            time_in_ready_queue = current_time - pstarv_ready_time;
            
            // In our simulation, boost after just 1 second to speed up demonstration
            if (time_in_ready_queue >= 1) {
                // Boost priority significantly - increment by 10 to ensure it runs soon
                pstarv->prprio = min(pstarv->prprio + 10, 50); 
                kprintf("\n*** TIMEBOOST: PStarv priority increased to %d after %d seconds in ready queue! ***\n\n",
                        pstarv->prprio, time_in_ready_queue);
                
                // Reset ready time
                pstarv_ready_time = current_time;
                
                // Force a resched to make sure the boosted priority is considered immediately
                if (pstarv->prprio > proctab[currpid].prprio) {
                    kprintf("PStarv now has higher priority than current process, forcing reschedule\n");
                    yield();
                }
            }
        }
    }
}

shellcmd starvation_test2(int nargs, char *args[]) {
    pid32 p1_pid_local, p2_pid_local;

    if (nargs > 1) {
        kprintf("Usage: starvation_test2\n");
        return SHELL_ERROR;
    }

    kprintf("===== STARTING Q2: TIME-BASED STARVATION PREVENTION =====\n");
    kprintf("Starting time-based starvation simulation at time %d...\n", clktime);

    // For Q2, we want time-based prevention, not context-switch-based
    enable_starvation_fix = FALSE;    // FALSE = time-based fix (Q2)
    pstarv_pid = BADPID;
    pstarv_ready_time = 0;
    last_boost_time = 0;

    // Create processes
    p1_pid_local = create(p1_func, 4096, 40, "P1_Process", 0);
    p2_pid_local = create(p2_func, 4096, 35, "P2_Process", 0);
    pstarv_pid   = create(pstarv_func, 4096, 25, "PStarv_Process", 0);

    if (p1_pid_local == SYSERR || p2_pid_local == SYSERR || pstarv_pid == SYSERR) {
        kprintf("Error: Failed to create one or more processes.\n");
        if (p1_pid_local != SYSERR) kill(p1_pid_local);
        if (p2_pid_local != SYSERR) kill(p2_pid_local);
        if (pstarv_pid != SYSERR) kill(pstarv_pid);

        enable_starvation_fix = FALSE;
        pstarv_pid = BADPID;
        return SHELL_ERROR;
    }

    kprintf("P1 created with PID: %d, Initial Priority: 40\n", p1_pid_local);
    kprintf("P2 created with PID: %d, Initial Priority: 35\n", p2_pid_local);
    kprintf("PStarv created with PID: %d, Initial Priority: 25\n", pstarv_pid);

    // Resume all processes - this puts them in the ready queue
    kprintf("\nResuming processes...\n");
    
    // Resume Pstarv first to ensure it's in the ready queue
    resume(pstarv_pid);
    // Record the time when Pstarv enters ready queue
    pstarv_ready_time = clktime;
    kprintf("PStarv resumed: state=%d, Ready time set to %d\n", 
            proctab[pstarv_pid].prstate, pstarv_ready_time);

    // Resume P2 next
    resume(p2_pid_local);
    kprintf("P2 resumed: state=%d\n", proctab[p2_pid_local].prstate);

    // Resume P1 last so it runs first (highest priority)
    resume(p1_pid_local);
    kprintf("P1 resumed: state=%d\n", proctab[p1_pid_local].prstate);

    kprintf("\nAll processes resumed. Demonstration parameters:\n");
    kprintf("- PStarv priority will boost every 1 second in ready queue\n");
    kprintf("- P1 and P2 will run for 5 iterations each\n");
    kprintf("- PStarv will run for 3 iterations\n");
    kprintf("- Current time is %d\n", clktime);
    kprintf("==========================================================\n\n");

    return SHELL_OK;
}