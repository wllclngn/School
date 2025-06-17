/* processes.c - Process implementations for starvation demonstration */

#include <xinu.h>
#include <pstarv.h>

/*------------------------------------------------------------------------
 *  process_p1  -  High priority process that runs in a loop
 *------------------------------------------------------------------------
 */
process process_p1(void)
{
    int i = 0;
    
    while (i < 30) {
        kprintf("P1 (PID: %d, Priority: %d) is running...\n", 
                currpid, getprio(currpid));
        
        /* Do some busy work */
        int j;
        for (j = 0; j < 1000000; j++) {
            /* Just waste some CPU cycles */
            if (j % 100000 == 0) {
                /* Yield periodically to ensure context switches */
                yield();
            }
        }
        
        i++;
    }
    
    kprintf("P1 (PID: %d) has completed.\n", currpid);
    return OK;
}

/*------------------------------------------------------------------------
 *  process_p2  -  High priority process that runs in a loop
 *------------------------------------------------------------------------
 */
process process_p2(void)
{
    int i = 0;
    
    while (i < 30) {
        kprintf("P2 (PID: %d, Priority: %d) is running...\n", 
                currpid, getprio(currpid));
        
        /* Do some busy work */
        int j;
        for (j = 0; j < 1000000; j++) {
            /* Just waste some CPU cycles */
            if (j % 100000 == 0) {
                /* Yield periodically to ensure context switches */
                yield();
            }
        }
        
        i++;
    }
    
    kprintf("P2 (PID: %d) has completed.\n", currpid);
    return OK;
}

/*------------------------------------------------------------------------
 *  process_pstarv  -  Process that might suffer starvation
 *------------------------------------------------------------------------
 */
process process_pstarv(void)
{
    /* This process will only run when its priority is high enough */
    kprintf("\n*** CELEBRATION! Pstarv (PID: %d, Priority: %d) is finally running! ***\n",
            currpid, getprio(currpid));
    kprintf("*** You'll get a good grade in CIS657! ***\n\n");
    
    /* Do some work to show it's running */
    int i, j;
    for (i = 0; i < 5; i++) {
        kprintf("Pstarv (PID: %d, Priority: %d) is running iteration %d\n",
                currpid, getprio(currpid), i);
        
        /* Some busy work */
        for (j = 0; j < 500000; j++) {
            /* Just waste CPU cycles */
        }
        
        /* Let other processes run */
        yield();
    }
    
    kprintf("Pstarv (PID: %d) has completed.\n", currpid);
    return OK;
}
