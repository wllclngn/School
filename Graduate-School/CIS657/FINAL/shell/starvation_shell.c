#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

extern void p1_func(void);
extern void p2_func(void);
extern void pstarv_func(void);

shellcmd starvation_test2(int nargs, char *args[]) {
    pid32 p1_pid_local, p2_pid_local; 

    if (nargs > 1) {
        kprintf("Usage: starvation_test2\n");
        return SHELL_ERROR;
    }

    kprintf("Starting time-based starvation simulation at time %d...\n", clktime);

    enable_starvation_fix = TRUE;
    pstarv_pid = BADPID;        // Global from pstarv.h
    pstarv_ready_time = 0;    // Global from pstarv.h
    last_boost_time = 0;      // Global from pstarv.h

    p1_pid_local = create(p1_func, 1024, 40, "P1_Process", 0);
    p2_pid_local = create(p2_func, 1024, 35, "P2_Process", 0);
    pstarv_pid = create(pstarv_func, 1024, 25, "Pstarv_Process", 0); // Assign to global

    if (p1_pid_local == SYSERR || p2_pid_local == SYSERR || pstarv_pid == SYSERR) {
        kprintf("Error: Failed to create one or more processes.\n");
        if (p1_pid_local != SYSERR) kill(p1_pid_local);
        if (p2_pid_local != SYSERR) kill(p2_pid_local);
        if (pstarv_pid != SYSERR) kill(pstarv_pid); // global
        
        enable_starvation_fix = FALSE;
        pstarv_pid = BADPID; // Reset global
        return SHELL_ERROR;
    }

    kprintf("P1 created with PID: %d, Initial Priority: 40\n", p1_pid_local);
    kprintf("P2 created with PID: %d, Initial Priority: 35\n", p2_pid_local);
    kprintf("Pstarv created with PID: %d, Initial Priority: 25\n", pstarv_pid); // global

    resume(p1_pid_local);
    resume(p2_pid_local);
    resume(pstarv_pid); // global

    kprintf("Processes resumed. Pstarv priority will boost every 2 seconds in ready queue.\n");
    kprintf("Current clock frequency: %d ticks per second\n", CLKTICKS_PER_SEC);

    return SHELL_OK;
}