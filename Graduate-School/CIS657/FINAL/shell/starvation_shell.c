/* starvation_shell.c - Modified for XINU Final Project
 * Last modified: 2025-06-15 16:01:30 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

extern void p1_func(void);
extern void p2_func(void);
// extern void pstarv_func(void); // Remove extern declaration, function is defined here

// New function for PStarv to check its ready time and boost priority
void check_pstarv_time(void) {
    uint32 current_time;
    uint32 time_in_ready_queue;

    if (enable_starvation_fix == FALSE && pstarv_pid != BADPID) {
        struct procent *pstarv = &proctab[pstarv_pid];
        if (pstarv->prstate == PR_READY) {
            current_time = clktime;
            time_in_ready_queue = current_time - pstarv_ready_time;

            if (time_in_ready_queue >= 2 * CLKTICKS_PER_SEC) {
                pstarv->prprio = min(pstarv->prprio + 5, 50); // Boost priority
                kprintf("TIMEBOOST: PStarv priority increased to %d after %d seconds\n",
                        pstarv->prprio, time_in_ready_queue / CLKTICKS_PER_SEC);
                pstarv_ready_time = current_time; // Reset ready time
            }
        }
    }
}

void pstarv_func(void) {
    kprintf("\n##########################################################################\n");
    kprintf("Pstarv (PID: %d, Prio: %d) HAS STARTED at time %d! Hooray!\n",
            currpid, proctab[currpid].prprio, clktime);
    kprintf("I (wllclngn) will get a good grade! Time-based scheduling works!\n");
    kprintf("##########################################################################\n\n");

    // Loop and check time periodically
    while (1) {
        check_pstarv_time();
        sleep(1); // Check every second
    }
}


shellcmd starvation_test2(int nargs, char *args[]) {  // Changed to match the name expected by shell.o
    pid32 p1_pid_local, p2_pid_local;

    if (nargs > 1) {
        kprintf("Usage: starvation_test2\n");  // Updated usage message
        return SHELL_ERROR;
    }

    kprintf("Starting time-based starvation simulation at time %d...\n", clktime);

    enable_starvation_fix = FALSE;    // Q2: time-based fix is FALSE
    pstarv_pid = BADPID;
    pstarv_ready_time = 0;
    last_boost_time = 0;

    // INCREASED STACK SIZE FOR SAFETY
    p1_pid_local = create(p1_func, 4096, 40, "P1_Process", 0);
    p2_pid_local = create(p2_func, 4096, 35, "P2_Process", 0);
    pstarv_pid   = create(pstarv_func, 4096, 25, "Pstarv_Process", 0);

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
    kprintf("Pstarv created with PID: %d, Initial Priority: 25\n", pstarv_pid);

    // Resume Pstarv first (recommended), then P1, then P2
    resume(pstarv_pid);
    kprintf("After resume(pstarv_pid): state=%d\n", proctab[pstarv_pid].prstate);

    resume(p1_pid_local);
    kprintf("After resume(p1_pid_local): state=%d\n", proctab[p1_pid_local].prstate);

    resume(p2_pid_local);
    kprintf("After resume(p2_pid_local): state=%d\n", proctab[p2_pid_local].prstate);

    kprintf("Processes resumed. Pstarv priority will boost every 2 seconds in ready queue.\n");
    kprintf("Current clock frequency: %d ticks per second\n", CLKTICKS_PER_SEC);

    return SHELL_OK;
}