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

    kprintf("========== XINU STARVATION TEST (v5) ==========\n");

    // Initialize starvation globals
    enable_starvation_fix = TRUE;
    kprintf("DEBUG: enable_starvation_fix set to %d (1=TRUE, 0=FALSE) at line %d\n", enable_starvation_fix, __LINE__);

    pstarv_pid = BADPID; // Initialize monitored PID for this test
    pstarv_ready_time = 0;
    last_boost_time = 0;

    // Create processes with robust stack size (4096)
    p1_pid_local = create(p1_func_q1, 4096, 40, "P1_Process", 0);
    kprintf("DEBUG: After create, P1 PID: %d, state: %d (Expected PR_SUSP=5)\n", p1_pid_local, proctab[p1_pid_local].prstate);

    p2_pid_local = create(p2_func_q1, 4096, 35, "P2_Process", 0);
    kprintf("DEBUG: After create, P2 PID: %d, state: %d (Expected PR_SUSP=5)\n", p2_pid_local, proctab[p2_pid_local].prstate);

    pstarv_pid = create(pstarv_func_q1, 4096, 25, "Pstarv_Process", 0); 
    kprintf("DEBUG: After create, Pstarv PID: %d, state: %d (Expected PR_SUSP=5)\n", pstarv_pid, proctab[pstarv_pid].prstate);

    // Check creation success
    if (p1_pid_local == SYSERR || p2_pid_local == SYSERR || pstarv_pid == SYSERR) {
        kprintf("ERROR: Failed to create one or more processes.\n");
        if (p1_pid_local != SYSERR) kill(p1_pid_local);
        if (p2_pid_local != SYSERR) kill(p2_pid_local);
        if (pstarv_pid != SYSERR) kill(pstarv_pid);

        enable_starvation_fix = TRUE;
        pstarv_pid = BADPID;
        return SHELL_ERROR;
    }

    // Resume processes (Pstarv first for test coverage)
    kprintf("DEBUG: Calling resume(pstarv_pid=%d)...\n", pstarv_pid);
    resume(pstarv_pid);
    kprintf("DEBUG: After resume(pstarv), state: %d (Expected PR_READY=2)\n", proctab[pstarv_pid].prstate);

    kprintf("DEBUG: Calling resume(p1_pid_local=%d)...\n", p1_pid_local);
    resume(p1_pid_local);
    kprintf("DEBUG: After resume(p1), state: %d (Expected PR_READY=2)\n", proctab[p1_pid_local].prstate);

    kprintf("DEBUG: Calling resume(p2_pid_local=%d)...\n", p2_pid_local);
    resume(p2_pid_local);
    kprintf("DEBUG: After resume(p2), state: %d (Expected PR_READY=2)\n", proctab[p2_pid_local].prstate);

    kprintf("INFO: All processes resumed. P1 and P2 should now run (watch for context switches and all process output).\n");

    kprintf("DEBUG: enable_starvation_fix is %d (1=TRUE, 0=FALSE) at line %d\n", enable_starvation_fix, __LINE__);

    if (enable_starvation_fix == TRUE) {
        kprintf("Q1 Starvation fix ENABLED: Pstarv's priority should be boosted at each context switch.\n");
    } else {
        kprintf("Q2 Starvation fix ENABLED: Pstarv's priority should increase based on time spent in ready state.\n");
    }

    kprintf("========== END OF SHELL SETUP ==========\n");
    return SHELL_OK;
}