#include <xinu.h>
#include <stdio.h>

void p2_func(void) {
    int i;
    for (i = 0; i < 20; i++) {
        kprintf("P2 (PID: %d, Prio: %d) is running (iteration %d) - Time: %d\n",
                currpid, proctab[currpid].prprio, i + 1, clktime);
        volatile int j;
        for(j=0; j<100000; j++);
        yield();
    }
    kprintf("P2 (PID: %d) finished.\n", currpid);
}