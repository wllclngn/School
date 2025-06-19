/* starvation_shell_q1.c - Shell commands for starvation tests (Q1) */

#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>
#include "shprototypes.h"

/*------------------------------------------------------------------------
 * starvation_test_Q1_entry - Shell command for Q1 test
 *------------------------------------------------------------------------
 */
shellcmd starvation_test_Q1_entry(int nargs, char *args[])
{
    pid32 p1_pid, p2_pid, pstarv_pid_local;

    if (nargs > 1) {
        kprintf("Usage: %s\n", args[0]);
        return SHELL_ERROR;
    }

    kprintf("\n=====================================================\n");
    kprintf("QUESTION 1: CONTEXT SWITCH-BASED STARVATION PREVENTION\n");
    kprintf("=====================================================\n\n");

    kprintf("Starting context switch-based starvation simulation...\n");

    /* Enable context switch-based starvation prevention */
    starvation_prevention = TRUE;
    starvingPID = BADPID;

    /* Create processes with different priorities */
    p1_pid = create(p1_func_q1, 4096, 40, "P1_Process_Q1", 0);
    p2_pid = create(p2_func_q1, 4096, 35, "P2_Process_Q1", 0);
    pstarv_pid_local = create(pstarv_func_q1_entry, 4096, 25, "PStarv_Process_Q1", 0);

    if (p1_pid == SYSERR || p2_pid == SYSERR || pstarv_pid_local == SYSERR) {
        kprintf("Error: Failed to create one or more processes.\n");
        if (p1_pid != SYSERR) kill(p1_pid);
        if (p2_pid != SYSERR) kill(p2_pid);
        if (pstarv_pid_local != SYSERR) kill(pstarv_pid_local);
        return SHELL_ERROR;
    }

    /* Set which process we want to monitor for starvation */
    starvingPID = pstarv_pid_local;
    kprintf("Set starvingPID to %d\n", starvingPID);

    /* Start the processes */
    resume(p1_pid);
    resume(p2_pid);
    resume(pstarv_pid_local);

    /* Let them run for a while */
    sleep(30);

    /* Clean up */
    kill(p1_pid);
    kill(p2_pid);
    kill(pstarv_pid_local);
    starvation_prevention = FALSE;

    kprintf("\n======================================================\n");
    kprintf("Context switch-based starvation simulation completed\n");
    kprintf("======================================================\n\n");

    return SHELL_OK;
}
