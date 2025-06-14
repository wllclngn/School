#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

void pstarv_func_q1(void) {
    kprintf("\n##########################################################################\n");
    kprintf("Pstarv (PID: %d, Prio: %d) IS FINALLY RUNNING! Hooray for fair scheduling!\n",
            currpid, proctab[currpid].prprio);
    kprintf("I (wllclngn) will get a good grade! This simulation rocks!\n");
    kprintf("##########################################################################\n\n");

    enable_starvation_fix = FALSE;
    pstarv_pid = BADPID;
}