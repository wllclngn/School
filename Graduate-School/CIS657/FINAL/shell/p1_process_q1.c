#include <xinu.h>
#include <stdio.h>

void p1_func_q1(void) {
    int i;
    for (i = 0; i < 15; i++) {
        kprintf("P1 (PID: %d, Prio: %d) is running (iteration %d)\n",
                currpid, proctab[currpid].prprio, i + 1);
        volatile int j;
        for(j=0; j<50000; j++);
        yield();
    }
    kprintf("P1 (PID: %d) finished.\n", currpid);
}