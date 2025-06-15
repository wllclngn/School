/* starvation_shell_q1.c - Modified for XINU Final Project
 * Last modified: 2025-06-15 06:14:32 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <stdio.h>
#include <pstarv.h>

extern void p1_func_q1(void);
extern void p2_func_q1(void);
extern void pstarv_func_q1(void);

shellcmd starvation_test(int nargs, char *args[]) {
    pid32 p1_pid, p2_pid; 

    if (nargs > 1) {
        kprintf("Usage: starvation_test\n");
        return SHELL_ERROR;
    }

    kprintf("Starting starvation simulation...\n");

    // Initialize global variables
    enable_starvation_fix = TRUE;    // Enable priority boosting
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

    kprintf("P1, P2, and PStarv processes created successfully\n");

    // Resume processes in strict priority order
    resume(p1_pid);    // Highest priority first
    sleep(1);         // Small delay between resumes
    resume(p2_pid);    // Medium priority second
    sleep(1);         // Small delay between resumes
    resume(pstarv_pid); // Lowest priority last

    kprintf("All processes resumed. Starting execution...\n");
    kprintf("=========== END OF SHELL SETUP ===========\n\n");

    return SHELL_OK;
}