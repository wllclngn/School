/* p2_process_q1.c - Higher priority process P2 for Q1 starvation simulation */

#include <xinu.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * p2_func_q1 - Entry function for the high-priority process P2 in Q1
 *------------------------------------------------------------------------
 */
void p2_func_q1(void)
{
    int iterations = 0;
    const int MAX_ITERATIONS = 50;
    
    kprintf("!!!!!!!!!! P2_Q1 (PID: %d, Prio: %d) HAS STARTED !!!!!!!!!!\n", 
            currpid, proctab[currpid].prprio);
    
    while (iterations < MAX_ITERATIONS) {
        kprintf("P2_Q1 (PID: %d, Prio: %d) is running (iteration %d/%d)\n",
                currpid, proctab[currpid].prprio, iterations + 1, MAX_ITERATIONS);
        
        /* Do some busy work to consume CPU time */
        volatile int j;
        for (j = 0; j < 500000; j++);
        
        iterations++;
        yield();
    }
    
    kprintf("P2_Q1 (PID: %d) finished.\n", currpid);
}
