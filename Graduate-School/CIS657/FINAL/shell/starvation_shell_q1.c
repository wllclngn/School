#include <xinu.h>
#include <pstarv.h> // Assuming this declares enable_starvation_fix, pstarv_pid
#include <stdio.h>
// You might need to include the header where PR_READY and PR_SUSP are defined,
// often process.h or kernel.h, if not already covered by xinu.h
// For example: #include <process.h>

extern void p1_func_q1(void);
extern void p2_func_q1(void);
extern void pstarv_func_q1(void);

shellcmd starvation_test(int nargs, char *args[]) {
    pid32 p1_pid_local, p2_pid_local; // Changed pid_t to pid32
    // pstarv_pid is already declared extern int, so we use it directly.

    // To print states meaningfully, you'd ideally use the macros.
    // For now, we'll just print the numbers.
    // From your include/process.h:
    // #define PR_SUSP 5
    // #define PR_READY 2

    if (nargs > 1) {
        kprintf("Usage: starvation_test\n");
        return SHELL_ERROR;
    }

    kprintf("Starting starvation simulation...\n");

    enable_starvation_fix = TRUE;
    pstarv_pid = BADPID; // Initialize global pstarv_pid

    p1_pid_local = create(p1_func_q1, 1024, 40, "P1_Process", 0);
    p2_pid_local = create(p2_func_q1, 1024, 35, "P2_Process", 0);
    // Assign the created PID to the global pstarv_pid
    pstarv_pid = create(pstarv_func_q1, 1024, 25, "Pstarv_Process", 0); 

    if (p1_pid_local == SYSERR || p2_pid_local == SYSERR || pstarv_pid == SYSERR) {
        kprintf("Error: Failed to create one or more processes.\n");
        if (p1_pid_local != SYSERR) kill(p1_pid_local);
        if (p2_pid_local != SYSERR) kill(p2_pid_local);
        if (pstarv_pid != SYSERR) kill(pstarv_pid);
        
        enable_starvation_fix = FALSE;
        pstarv_pid = BADPID; // Reset on error
        return SHELL_ERROR;
    }

    kprintf("P1 created with PID: %d, Initial Priority: 40\n", p1_pid_local);
    kprintf("P2 created with PID: %d, Initial Priority: 35\n", p2_pid_local);
    kprintf("Pstarv created with PID: %d, Initial Priority: 25. This PID will be monitored.\n", pstarv_pid);

    // Check states immediately after creation
    kprintf("SHELL: P1 (PID: %d) state after create: %d (Expected PR_SUSP = 5)\n",
            p1_pid_local, proctab[p1_pid_local].prstate);
    kprintf("SHELL: P2 (PID: %d) state after create: %d (Expected PR_SUSP = 5)\n",
            p2_pid_local, proctab[p2_pid_local].prstate);
    kprintf("SHELL: Pstarv (PID: %d) state after create: %d (Expected PR_SUSP = 5)\n",
            pstarv_pid, proctab[pstarv_pid].prstate);

    // Resume Pstarv FIRST and check its state immediately
    kprintf("SHELL: Calling resume(pstarv_pid (%d))...\n", pstarv_pid);
    resume(pstarv_pid);
    kprintf("SHELL: After resume(pstarv_pid (%d)), Pstarv state is: %d (Expected PR_READY = 2)\n",
            pstarv_pid, proctab[pstarv_pid].prstate);

    // Then resume P1 and P2
    kprintf("SHELL: Calling resume(p1_pid_local (%d)) and resume(p2_pid_local (%d))...\n", p1_pid_local, p2_pid_local);
    resume(p1_pid_local);
    resume(p2_pid_local);

    kprintf("Processes P1, P2, and Pstarv have been resumed.\n");
    kprintf("P1 and P2 will run, causing context switches.\n");
    if (enable_starvation_fix == TRUE) {
        kprintf("Q1 Starvation fix is ENABLED. Pstarv's priority should be boosted at each context switch (if it's ready and not the one switching out).\n");
    } else {
        kprintf("Q2 Starvation fix is (presumably) ENABLED. Pstarv's priority should be boosted based on time.\n");
    }
    kprintf("Watch the output for Pstarv's priority increasing and relevant DEBUG messages...\n");

    return SHELL_OK;
}