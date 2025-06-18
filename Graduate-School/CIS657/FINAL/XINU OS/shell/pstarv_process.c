/* pstarv_process.c - PStarv process for XINU Final Project
 */

#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

void pstarv_func(void) {
    kprintf("\n##########################################################################\n");
    kprintf("PStarv (PID: %d, Prio: %d) IS FINALLY RUNNING at time %d! Hooray!\n",
            currpid, proctab[currpid].prprio, clktime);
    kprintf("Time-based scheduling works!\n");
    kprintf("##########################################################################\n\n");

    // Run a few iterations to show it's working
    int i;
    const int MAX_ITERATIONS = 3;  // Just 3 iterations to keep output concise
    
    for (i = 1; i <= MAX_ITERATIONS; i++) {
        kprintf("PStarv (PID: %d, Prio: %d) running iteration %d/%d - Time: %d\n",
                currpid, proctab[currpid].prprio, i, MAX_ITERATIONS, clktime);
        
        // Short delay
        volatile unsigned int j;
        for(j = 0; j < 15000; j++);
        
        yield();
    }
    
    kprintf("PStarv (PID: %d) FINISHED ALL ITERATIONS.\n", currpid);
    
    // Reset variables since starvation has been resolved
    kprintf("\n##########################################################################\n");
    kprintf("Time-based starvation prevention demonstration completed successfully!\n");
    kprintf("##########################################################################\n\n");
    
    enable_starvation_fix = TRUE;
    pstarv_pid = BADPID;
    
    // Process terminates
    kill(currpid);
}