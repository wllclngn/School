#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

extern void p1_func_q1(void);
extern void p2_func_q1(void);
extern void pstarv_func_q1(void);

shellcmd starvation_test(int nargs, char *args[]) {
    pid_t p1_pid_local, p2_pid_local;

    if (nargs > 1) {
        kprintf("Usage: starvation_test\n");
        return SHELL_ERROR;
    }

    kprintf("Starting starvation simulation...\n");

    enable_starvation_fix = TRUE;
    pstarv_pid = BADPID;

    p1_pid_local = create(p1_func_q1, 1024, 40, "P1_Process", 0);
    p2_pid_local = create(p2_func_q1, 1024, 35, "P2_Process", 0);
    pstarv_pid = create(pstarv_func_q1, 1024, 25, "Pstarv_Process", 0);

    if (p1_pid_local == SYSERR || p2_pid_local == SYSERR || pstarv_pid == SYSERR) {
        kprintf("Error: Failed to create one or more processes.\n");
        if (p1_pid_local != SYSERR) kill(p1_pid_local);
        if (p2_pid_local != SYSERR) kill(p2_pid_local);
        if (pstarv_pid != SYSERR) kill(pstarv_pid);
        
        enable_starvation_fix = FALSE;
        pstarv_pid = BADPID;
        return SHELL_ERROR;
    }

    kprintf("P1 created with PID: %d, Initial Priority: 40\n", p1_pid_local);
    kprintf("P2 created with PID: %d, Initial Priority: 35\n", p2_pid_local);
    kprintf("Pstarv created with PID: %d, Initial Priority: 25. This PID will be monitored.\n", pstarv_pid);

    resume(p1_pid_local);
    resume(p2_pid_local);
    resume(pstarv_pid);

    kprintf("Processes resumed. P1 and P2 will run, causing context switches.\n");
    kprintf("Pstarv's priority will be boosted at each context switch until it runs.\n");
    kprintf("Watch the output for Pstarv's priority increasing...\n");

    return SHELL_OK;
}