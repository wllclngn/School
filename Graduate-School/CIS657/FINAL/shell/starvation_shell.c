/* starvation_shell_q1.c - Modified for XINU Final Project
 * Last modified: 2025-06-15 05:08:02 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <pstarv.h> 
#include <stdio.h>

extern void p1_func_q1(void);
extern void p2_func_q1(void);
extern void pstarv_func_q1(void);

shellcmd starvation_test(int nargs, char *args[]) {
    pid32 p1_pid_local, p2_pid_local; 

    if (nargs > 1) {
        kprintf("Usage: starvation_test\n");
        return SHELL_ERROR;
    }

    kprintf("\n========== XINU Process Scheduling Simulation ==========\n");
    kprintf("Demonstrating process starvation prevention...\n");

    // Initialize globals
    enable_starvation_fix = TRUE;
    pstarv_pid = BADPID;

    // Create processes with defined priorities
    p1_pid_local = create(p1_func_q1, 4096, 40, "P1", 0);
    p2_pid_local = create(p2_func_q1, 4096, 35, "P2", 0);
    pstarv_pid = create(pstarv_func_q1, 4096, 25, "PStarv", 0);

    if (p1_pid_local == SYSERR || p2_pid_local == SYSERR || pstarv_pid == SYSERR) {
        kprintf("Process creation failed\n");
        return SHELL_ERROR;
    }

    kprintf("Created: P1(pid=%d, pri=40), P2(pid=%d, pri=35), PStarv(pid=%d, pri=25)\n", 
            p1_pid_local, p2_pid_local, pstarv_pid);

    // Start processes
    resume(p1_pid_local);
    resume(p2_pid_local);
    resume(pstarv_pid);

    return SHELL_OK;
}