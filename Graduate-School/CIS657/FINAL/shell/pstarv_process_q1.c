#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

void pstarv_func_q1(void) {
    kprintf("\n##########################################################################\n");
    kprintf("Pstarv (PID: %d, Prio: %d) IS FINALLY RUNNING! Hooray for fair scheduling!\n",
            currpid, proctab[currpid].prprio);
    kprintf("I (wllclngn) will get a good grade! This simulation rocks!\n");
    kprintf("##########################################################################\n\n");

    int i;
    for (i = 0; i < 100; i++) {  // Added 100 iteration loop
        kprintf("PStarv (PID: %d, Prio: %d) is running (iteration %d/100)\n",
                currpid, proctab[currpid].prprio, i + 1);
        volatile int j;
        for(j=0; j<5; j++);
        yield();
    }
    kprintf("PStarv (PID: %d) finished.\n", currpid);
}