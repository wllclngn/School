#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

void pstarv_func(void) {
    kprintf("\n##########################################################################\n");
    kprintf("Pstarv (PID: %d, Prio: %d) IS FINALLY RUNNING at time %d! Hooray!\n",
            currpid, proctab[currpid].prprio, clktime);
    kprintf("I (wllclngn) will get a good grade! Time-based scheduling works!\n");
    kprintf("##########################################################################\n\n");

    enable_starvation_fix = FALSE;
    pstarv_pid = BADPID;
    pstarv_ready_time = 0;
    last_boost_time = 0;
}