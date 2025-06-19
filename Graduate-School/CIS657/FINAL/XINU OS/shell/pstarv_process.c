/* pstarv_process.c - Process for demonstrating starvation prevention */

#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * pstarv_func_q1 - Pstarv function for Question 1 (context switch-based)
 *------------------------------------------------------------------------
 */
void pstarv_func_q1(void)
{
    int i;
    
    kprintf("\n##########################################################################\n");
    kprintf("Pstarv (PID: %d, Prio: %d) HAS STARTED at time %d! Hooray!\n",
            currpid, proctab[currpid].prprio, clktime);
    kprintf("Context switch-based starvation prevention works!\n");
    kprintf("##########################################################################\n\n");
    
    /* Do some work to show it's running */
    for (i = 0; i < 10; i++) {
        kprintf("Pstarv Q1 (PID: %d, Priority: %d) is running iteration %d\n",
                currpid, proctab[currpid].prprio, i);
        
        /* Some busy work */
        volatile int j;
        for (j = 0; j < 500000; j++);
        
        /* Let other processes run */
        yield();
    }
    
    kprintf("Pstarv Q1 (PID: %d) has completed its execution.\n", currpid);
}

/*------------------------------------------------------------------------
 * pstarv_func_q2 - Pstarv function for Question 2 (time-based)
 *------------------------------------------------------------------------
 */
void pstarv_func_q2(void)
{
    int i;
    
    kprintf("\n##########################################################################\n");
    kprintf("Pstarv (PID: %d, Prio: %d) HAS STARTED at time %d! Hooray!\n",
            currpid, proctab[currpid].prprio, clktime);
    kprintf("Time-based starvation prevention works!\n");
    kprintf("##########################################################################\n\n");
    
    /* Do some work to show it's running */
    for (i = 0; i < 10; i++) {
        kprintf("Pstarv Q2 (PID: %d, Priority: %d) is running iteration %d\n",
                currpid, proctab[currpid].prprio, i);
        
        /* Some busy work */
        volatile int j;
        for (j = 0; j < 500000; j++);
        
        /* Let other processes run */
        yield();
    }
    
    kprintf("Pstarv Q2 (PID: %d) has completed its execution.\n", currpid);
}
