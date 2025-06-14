#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

void p1_func(void);
void p2_func(void);
void pstarv_func(void);

void p1_func(void) {
    int i;
    for (i = 0; i < 15; i++) {
        kprintf("P1 (PID: %d, Prio: %d) is running (iteration %d)\n",
                currpid, proctab[currpid].prprio, i + 1);
        volatile int j; // Simple delay
        for(j=0; j<50000; j++);
        yield();
    }
    kprintf("P1 (PID: %d) finished.\n", currpid);
}

void p2_func(void) {
    int i;
    for (i = 0; i < 15; i++) {
        kprintf("P2 (PID: %d, Prio: %d) is running (iteration %d)\n",
                currpid, proctab[currpid].prprio, i + 1);
        volatile int j; // Simple delay
        for(j=0; j<50000; j++);
        yield();
    }
    kprintf("P2 (PID: %d) finished.\n", currpid);
}

void pstarv_func(void) {
    kprintf("\n##########################################################################\n");
    kprintf("Pstarv (PID: %d, Prio: %d) IS FINALLY RUNNING! Hooray for fair scheduling!\n",
            currpid, proctab[currpid].prprio);
    kprintf("I (wllclngn) will get a good grade! This simulation rocks!\n");
    kprintf("##########################################################################\n\n");

    enable_starvation_fix = FALSE;
    pstarv_pid = BADPID;
}

shellcmd starvation_test(int nargs, char *args[]) {
    pid_t p1_pid_local, p2_pid_local;

    if (nargs > 1) {
        kprintf("Usage: starvation_test\n");
        return SHELL_ERROR;
    }

    kprintf("Starting starvation simulation...\n");

    enable_starvation_fix = TRUE;
    pstarv_pid = BADPID;

    p1_pid_local = create(p1_func, 1024, 40, "P1_Process", 0);
    p2_pid_local = create(p2_func, 1024, 35, "P2_Process", 0);
    pstarv_pid = create(pstarv_func, 1024, 25, "Pstarv_Process", 0);

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