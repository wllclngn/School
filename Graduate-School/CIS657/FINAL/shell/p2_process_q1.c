
#include <xinu.h>
#include <stdio.h>

void p2_func_q1(void) {
    int i;
    for (i = 0; i < 100; i++) {  // Changed to 100 iterations
        kprintf("P2 (PID: %d, Prio: %d) is running (iteration %d/100)\n",
                currpid, proctab[currpid].prprio, i + 1);
        volatile int j;
        for(j=0; j<5; j++);
        yield();
    }
    kprintf("P2 (PID: %d) finished.\n", currpid);
}