/* p1_process_q1.c - Modified for XINU Final Project
 * Last modified: 2025-06-15 06:42:05 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <stdio.h>

void p1_func_q1(void) {
    int iterations = 0;
    const int MAX_ITERATIONS = 50;
    
    kprintf("!!!!!!!!!! P1_func_q1 (PID: %d, Prio: %d) HAS STARTED !!!!!!!!!!\n", 
            currpid, proctab[currpid].prprio);
    
    while (iterations < MAX_ITERATIONS) {
        kprintf("P1 (PID: %d, Prio: %d) is running (iteration %d/%d)\n",
                currpid, proctab[currpid].prprio, iterations + 1, MAX_ITERATIONS);
        
        // Longer delay loop
        volatile int j;
        for(j = 0; j < 500000; j++);
        
        iterations++;
        yield();
    }
    
    kprintf("P1 (PID: %d) finished.\n", currpid);
}