/* pstarv_process_q1.c - Process for demonstrating Q1 starvation prevention */

#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * pstarv_func_q1 - Pstarv function for Question 1 (context switch-based)
 *------------------------------------------------------------------------
 */
void pstarv_func_q1_entry(void)
{
    int i;
    
    kprintf("\n##########################################################################\n");
    kprintf("Pstarv_Q1 (PID: %d, Prio: %d) HAS STARTED at time %d! Hooray!\n",
            currpid, proctab[currpid].prprio, clktime);
    kprintf("Context switch-based starvation prevention works!\n");
    kprintf("##########################################################################\n\n");
    
    /* Do some work to show it's running */
    for (i = 0; i < 10; i++) {
        kprintf("Pstarv_Q1 (PID: %d, Priority: %d) is running iteration %d\n",
                currpid, proctab[currpid].prprio, i);
        
        /* Some busy work */
        volatile int j;
        for (j = 0; j < 500000; j++);
        
        /* Let other processes run */
        yield();
    }
    
    kprintf("Pstarv_Q1 (PID: %d) has completed its execution.\n", currpid);
}
