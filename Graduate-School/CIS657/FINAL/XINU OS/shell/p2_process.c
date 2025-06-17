/* p2_process.c - P2 process for XINU Final Project
 */

#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

void p2_func(void) {
    int i;
    const int MAX_ITERATIONS = 5;  // Limited to 5 iterations for demonstration
    
    for (i = 1; i <= MAX_ITERATIONS; i++) {
        kprintf("P2 (PID: %d, Prio: %d) running iteration %d/%d - Time: %d\n",
                currpid, proctab[currpid].prprio, i, MAX_ITERATIONS, clktime);
        
        // Shorter busy loop to consume less CPU time
        volatile unsigned int j;
        for(j = 0; j < 25000; j++);
        
        // Yield after each iteration to allow other processes to run
        yield();
    }
    
    kprintf("P2 (PID: %d) FINISHED ALL ITERATIONS.\n", currpid);
    // Process terminates
    kill(currpid);
}