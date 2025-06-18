/* pstarv_process_q1.c - Modified for XINU Final Project
 * Last modified: 2025-06-16 00:05:22 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <stdio.h>

void pstarv_func_q1(void) {
    int iterations = 0;
    const int MAX_ITERATIONS = 25;
/* Make sure the process identifies itself correctly */

    kprintf("\n##########################################################################\n");
    kprintf("PStarv (PID: %d, Prio: %d) IS FINALLY RUNNING! Priority boosting works!\n",
            currpid, proctab[currpid].prprio);
    kprintf("XINU is awesome!\n");
    kprintf("##########################################################################\n\n");
    
    while (iterations < MAX_ITERATIONS) {
        kprintf("PStarv (PID: %d, Prio: %d) running iteration %d/%d\n",
                currpid, proctab[currpid].prprio, iterations + 1, MAX_ITERATIONS);
        
        // Longer delay loop but slightly shorter than P1/P2
        volatile int j;
        for(j = 0; j < 400000; j++);
        
        iterations++;
        yield();
    }
    
    kprintf("PStarv (PID: %d) finished.\n", currpid);
}