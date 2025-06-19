/* starvation_shell.c - Shell commands for starvation tests */

#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>
#include "shprototypes.h"

/*------------------------------------------------------------------------
 * starvation_test_Q1 - Shell command to run the context switch-based starvation test (Q1)
 *------------------------------------------------------------------------
 */
shellcmd starvation_test_Q1(int nargs, char *args[])
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
    p1_pid = create(p1_func, 4096, 40, "P1_Process", 0);
    p2_pid = create(p2_func, 4096, 35, "P2_Process", 0);
    pstarv_pid_local = create(pstarv_func_q1, 4096, 25, "PStarv_Process", 0);

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

/*------------------------------------------------------------------------
 * starvation_test_Q2 - Shell command to run the time-based starvation test (Q2)
 *------------------------------------------------------------------------
 */
shellcmd starvation_test_Q2(int nargs, char *args[])
{
    pid32 p1_pid_local, p2_pid_local;

    if (nargs > 1) {
        kprintf("Usage: %s\n", args[0]);
        return SHELL_ERROR;
    }

    kprintf("\n\n=====================================================\n");
    kprintf("QUESTION 2: TIME-BASED STARVATION PREVENTION\n");
    kprintf("=====================================================\n\n");

    kprintf("Starting time-based starvation simulation at time %d...\n", clktime);

    /* Set up for time-based starvation prevention */
    enable_starvation_fix = FALSE;  /* We'll use our own timer mechanism */
    pstarv_pid = BADPID;
    pstarv_ready_time = 0;
    last_boost_time = 0;

    /* Create processes with different priorities */
    p1_pid_local = create(p1_func, 4096, 40, "P1_Process_Q2", 0);
    p2_pid_local = create(p2_func, 4096, 35, "P2_Process_Q2", 0);
    pstarv_pid = create(pstarv_func_q2, 4096, 25, "Pstarv_Process_Q2", 0);

    if (p1_pid_local == SYSERR || p2_pid_local == SYSERR || pstarv_pid == SYSERR) {
        kprintf("Error: Failed to create one or more processes.\n");
        if (p1_pid_local != SYSERR) kill(p1_pid_local);
        if (p2_pid_local != SYSERR) kill(p2_pid_local);
        if (pstarv_pid != SYSERR) kill(pstarv_pid);
        return SHELL_ERROR;
    }

    /* Record the time when Pstarv enters ready queue */
    pstarv_ready_time = clktime;
    kprintf("Initializing pstarv_ready_time to %d\n", pstarv_ready_time);

    /* Start the processes */
    resume(p1_pid_local);
    resume(p2_pid_local);
    resume(pstarv_pid);

    /* Let them run for a while */
    sleep(30);

    /* Clean up */
    kill(p1_pid_local);
    kill(p2_pid_local);
    kill(pstarv_pid);

    kprintf("\n======================================================\n");
    kprintf("Time-based starvation simulation completed\n");
    kprintf("======================================================\n\n");

    return SHELL_OK;
}
