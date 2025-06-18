/* starvation_shell_q1.c - Modified for XINU Final Project */

#include <xinu.h>
#include <stdio.h>
#include <pstarv.h>

extern void p1_func_q1(void);
extern void p2_func_q1(void);
extern void pstarv_func_q1(void);

shellcmd starvation_test(int nargs, char *args[]) {
    pid32 p1_pid, p2_pid;

    if (nargs > 1) {
        kprintf("Usage: starvation_test_Q1\n");
        return SHELL_ERROR;
    }

    kprintf("\n===== STARTING Q1: CONTEXT-SWITCH BASED STARVATION PREVENTION =====\n");
    kprintf("Starting context-switch-based starvation simulation...\n");

    // Initialize global variables
    enable_starvation_fix = TRUE;    // Enable priority boosting on context switch
    pstarv_pid = BADPID;            // Initialize to invalid PID

    // Create processes with proper priorities
    p1_pid = create(p1_func_q1, 4096, 40, "P1_Process", 0);
    p2_pid = create(p2_func_q1, 4096, 35, "P2_Process", 0);
    pstarv_pid = create(pstarv_func_q1, 4096, 25, "PStarv_Process", 0);

    if (p1_pid == SYSERR || p2_pid == SYSERR || pstarv_pid == SYSERR) {
        kprintf("Error creating processes\n");
        if (p1_pid != SYSERR) kill(p1_pid);
        if (p2_pid != SYSERR) kill(p2_pid);
        if (pstarv_pid != SYSERR) kill(pstarv_pid);
        return SHELL_ERROR;
    }

    kprintf("P1 created with PID: %d, Initial Priority: 40\n", p1_pid);
    kprintf("P2 created with PID: %d, Initial Priority: 35\n", p2_pid);
    kprintf("PStarv created with PID: %d, Initial Priority: 25\n", pstarv_pid);
    kprintf("\nQ1 DEMONSTRATION SETTINGS:\n");
    kprintf("- PStarv priority will increase by 2 with each context switch\n");
    kprintf("- Context switches will occur between P1 and P2\n");
    kprintf("- Eventually PStarv's priority will be high enough to run\n\n");

    // Resume processes in strict priority order
    resume(p1_pid);    // Highest priority first
    sleep(1);         // Small delay between resumes
    resume(p2_pid);    // Medium priority second
    sleep(1);         // Small delay between resumes
    resume(pstarv_pid); // Lowest priority last

    kprintf("All processes resumed. Starting execution...\n");
    kprintf("===============================================================\n\n");

    return SHELL_OK;
}