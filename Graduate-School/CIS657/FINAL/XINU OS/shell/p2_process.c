/* p2_process.c - Higher priority process P2 for starvation simulation */

#include <xinu.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * p2_func - Entry function for the high-priority process P2
 *------------------------------------------------------------------------
 */
void p2_func(void)
{
    int iterations = 0;
    const int MAX_ITERATIONS = 50;
    
    kprintf("!!!!!!!!!! P2 (PID: %d, Prio: %d) HAS STARTED !!!!!!!!!!\n", 
            currpid, proctab[currpid].prprio);
    
    while (iterations < MAX_ITERATIONS) {
        kprintf("P2 (PID: %d, Prio: %d) is running (iteration %d/%d)\n",
                currpid, proctab[currpid].prprio, iterations + 1, MAX_ITERATIONS);
        
        /* Do some busy work to consume CPU time */
        volatile int j;
        for (j = 0; j < 500000; j++);
        
        iterations++;
        yield();
    }
    
    kprintf("P2 (PID: %d) finished.\n", currpid);
}
