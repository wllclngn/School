/* starvation_shell_q1.c - modified for XINU Final Project
 * Last modified: 2025-06-15 04:23:18 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <pstarv.h> 
#include <stdio.h>

// Function prototypes for your test processes
extern void p1_func_q1(void);
extern void p2_func_q1(void);
extern void pstarv_func_q1(void);

shellcmd starvation_test(int nargs, char *args[]) {
    pid32 p1_pid_local, p2_pid_local; 

    if (nargs > 1) {
        kprintf("Usage: starvation_test\n");
        return SHELL_ERROR;
    }

    kprintf("========== XINU STARVATION TEST (v7) ==========\n");

    // Initialize starvation globals
    enable_starvation_fix = TRUE;
    kprintf("DEBUG: enable_starvation_fix set to %d (1=TRUE, 0=FALSE)\n", enable_starvation_fix);

    pstarv_pid = BADPID;
    pstarv_ready_time = 0;
    last_boost_time = 0;

    // CRITICAL CHANGE - Temporarily lower shell priority
    proctab[currpid].prprio = 20;  // Lower shell priority to allow other processes to run

    // Create processes with robust stack size (4096)
    p1_pid_local = create(p1_func_q1, 4096, 40, "P1_Process", 0);
    p2_pid_local = create(p2_func_q1, 4096, 35, "P2_Process", 0);
    pstarv_pid = create(pstarv_func_q1, 4096, 25, "Pstarv_Process", 0);

    // Verify process creation
    kprintf("\nProcess States after creation:\n");
    kprintf("P1 (PID:%d): Priority=%d State=%d\n", 
            p1_pid_local, proctab[p1_pid_local].prprio, proctab[p1_pid_local].prstate);
    kprintf("P2 (PID:%d): Priority=%d State=%d\n", 
            p2_pid_local, proctab[p2_pid_local].prprio, proctab[p2_pid_local].prstate);
    kprintf("PStarv (PID:%d): Priority=%d State=%d\n", 
            pstarv_pid, proctab[pstarv_pid].prprio, proctab[pstarv_pid].prstate);

    // Check creation success
    if (p1_pid_local == SYSERR || p2_pid_local == SYSERR || pstarv_pid == SYSERR) {
        kprintf("ERROR: Failed to create one or more processes.\n");
        if (p1_pid_local != SYSERR) kill(p1_pid_local);
        if (p2_pid_local != SYSERR) kill(p2_pid_local);
        if (pstarv_pid != SYSERR) kill(pstarv_pid);

        // Restore shell priority before returning
        proctab[currpid].prprio = 50;
        enable_starvation_fix = TRUE;
        pstarv_pid = BADPID;
        return SHELL_ERROR;
    }

    // Resume processes in specific order
    kprintf("\nResuming processes (each will run for 100 iterations)...\n");
    
    // CRITICAL CHANGE - Force a resched after each resume
    resume(pstarv_pid);
    resched();
    kprintf("DEBUG: After resume(pstarv), state: %d (Expected PR_READY=2)\n", 
            proctab[pstarv_pid].prstate);

    resume(p1_pid_local);
    resched();
    kprintf("DEBUG: After resume(p1), state: %d (Expected PR_READY=2)\n", 
            proctab[p1_pid_local].prstate);

    resume(p2_pid_local);
    resched();
    kprintf("DEBUG: After resume(p2), state: %d (Expected PR_READY=2)\n", 
            proctab[p2_pid_local].prstate);

    // Final verification
    kprintf("\nAll processes in ready state. Starting execution...\n");
    
    // CRITICAL CHANGE - Force immediate scheduling of highest priority process
    resched();

    // Restore shell priority
    proctab[currpid].prprio = 50;

    kprintf("========== END OF SHELL SETUP ==========\n");
    return SHELL_OK;
}