#include <xinu.h>
#include <stdio.h>
#include <string.h>

// Declarations for global variables that will be defined in initialize.c
// and declared as extern in externs.h
extern int g_enable_starvation_fix;
extern pid32 g_pstarv_pid;
extern uint32 g_pstarv_ready_time;
extern uint32 g_last_boost_time;

// Process P1
void P1_func(void) {
    int i;
    for (i = 0; i < 15; i++) {
        kprintf("P1 (PID: %d, Prio: %d) is running (iteration %d)\n",
                currpid, proctab[currpid].prprio, i + 1);
        sleepms(200); // Cause context switch
    }
    kprintf("P1 (PID: %d) finished.\n", currpid);
}

// Process P2
void P2_func_q1(void) {
    int i;
    kprintf("!!! P2_func_q1 (PID:%d, Prio: %d) HAS STARTED !!!\n", currpid, proctab[currpid].prprio);
    for (i = 0; i < 15; i++) {
        kprintf("P2 (PID: %d, Prio: %d) is running (iteration %d)\n",
                currpid, proctab[currpid].prprio, i + 1);
        sleepms(200); // Cause context switch
    }
    kprintf("P2 (PID: %d) finished.\n", currpid);
}

// Process Pstarv
void Pstarv_func(void) {
    kprintf("\n\n********************************************************\n");
    kprintf("Pstarv (PID: %d, Prio: %d) IS FINALLY RUNNING!!!\n", currpid, proctab[currpid].prprio);
    kprintf("Celebrating a good grade on the final exam!\n");
    kprintf("********************************************************\n\n");
}

shellcmd xsh_final_test(int nargs, char *args[]) {
    pid32 p1_pid, p2_pid, pstarv_local_pid;

    if (nargs == 2 && strncmp(args[1], "q2", 2) == 0) {
        g_enable_starvation_fix = FALSE; // Enable Q2 logic
        kprintf("Shell: Starvation fix Q2 (time-based) ENABLED.\n");
    } else {
        g_enable_starvation_fix = TRUE;  // Enable Q1 logic
        kprintf("Shell: Starvation fix Q1 (context-switch-based) ENABLED.\n");
    }

    g_pstarv_pid = BADPID;
    g_pstarv_ready_time = 0;
    g_last_boost_time = clktime;

    kprintf("Shell: Creating processes for the starvation test...\n");

    p1_pid = create(P1_func, INITSTK, 40, "P1_Process", 0);
    if (p1_pid == SYSERR) {
        kprintf("Shell: Failed to create P1. Aborting.\n");
        return SYSERR;
    }
    kprintf("Shell: P1 created with PID: %d, Priority: 40\n", p1_pid);

    p2_pid = create(P2_func_q1, INITSTK, 35, "P2_Process", 0);
    if (p2_pid == SYSERR) {
        kprintf("Shell: Failed to create P2. Aborting.\n");
        kill(p1_pid);
        return SYSERR;
    }
    kprintf("Shell: P2 created with PID: %d, Priority: 35\n", p2_pid);

    pstarv_local_pid = create(Pstarv_func, INITSTK, 25, "Pstarv_Process", 0);
    if (pstarv_local_pid == SYSERR) {
        kprintf("Shell: Failed to create Pstarv. Aborting.\n");
        kill(p1_pid);
        kill(p2_pid);
        return SYSERR;
    }
    kprintf("Shell: Pstarv created with PID: %d, Priority: 25\n", pstarv_local_pid);

    g_pstarv_pid = pstarv_local_pid;
    kprintf("Shell: Global g_pstarv_pid set to %d\n", g_pstarv_pid);

    kprintf("Shell: Resuming P1, P2, and Pstarv...\n");
    resume(p1_pid);
    resume(p2_pid);
    resume(pstarv_local_pid);

    kprintf("Shell: Processes resumed. Monitor console for output.\n");
    kprintf("-------------------------------------------------------\n");

    return SHELL_OK;
}