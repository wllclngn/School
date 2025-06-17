/* pstarv.c - Process starvation prevention mechanisms */

#include <xinu.h>
#include <pstarv.h>

/* Global variables for starvation prevention */
pid32 pstarv_pid = BADPID;        /* PID of the process that might suffer starvation */
bool8 enable_starvation_fix = FALSE; /* Flag to enable/disable the starvation fix */

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

/*------------------------------------------------------------------------
 *  starvation_q1_init  -  Initialize Q1 starvation prevention (context switch based)
 *------------------------------------------------------------------------
 */
syscall starvation_q1_init(void)
{
    /* Enable the starvation fix for Q1 (context switch based) */
    enable_starvation_fix = TRUE;
    
    /* Create high priority processes P1 and P2 */
    pid32 p1_pid = create("P1", process_p1, 1024, 40, "P1", 0);
    pid32 p2_pid = create("P2", process_p2, 1024, 35, "P2", 0);
    
    /* Create the starving process Pstarv with lower priority */
    pstarv_pid = create("Pstarv", process_pstarv, 1024, 25, "Pstarv", 0);
    
    /* Make sure processes were created successfully */
    if ((p1_pid == SYSERR) || (p2_pid == SYSERR) || (pstarv_pid == SYSERR)) {
        kprintf("Error: Failed to create processes for starvation test\n");
        return SYSERR;
    }
    
    /* Start the processes */
    resume(p1_pid);
    resume(p2_pid);
    resume(pstarv_pid);
    
    return OK;
}

/*------------------------------------------------------------------------
 *  starvation_q2_init  -  Initialize Q2 starvation prevention (time based)
 *------------------------------------------------------------------------
 */
syscall starvation_q2_init(void)
{
    /* Disable the context switch based fix (Q1) and enable time-based (Q2) */
    enable_starvation_fix = FALSE;
    
    /* Create high priority processes P1 and P2 */
    pid32 p1_pid = create("P1", process_p1, 1024, 40, "P1", 0);
    pid32 p2_pid = create("P2", process_p2, 1024, 35, "P2", 0);
    
    /* Create the starving process Pstarv with lower priority */
    pstarv_pid = create("Pstarv", process_pstarv, 1024, 25, "Pstarv", 0);
    
    /* Make sure processes were created successfully */
    if ((p1_pid == SYSERR) || (p2_pid == SYSERR) || (pstarv_pid == SYSERR)) {
        kprintf("Error: Failed to create processes for starvation test\n");
        return SYSERR;
    }
    
    /* Start the processes */
    resume(p1_pid);
    resume(p2_pid);
    resume(pstarv_pid);
    
    return OK;
}

/*------------------------------------------------------------------------
 *  run_starvation_test_q1  -  Run starvation test for Q1 (context switch based)
 *------------------------------------------------------------------------
 */
process run_starvation_test_q1(void)
{
    kprintf("\n*** Starting Starvation Test Q1 (Context Switch Based) ***\n\n");
    kprintf("Creating processes P1 (priority 40), P2 (priority 35), and Pstarv (priority 25)\n");
    kprintf("Pstarv's priority will be increased by 2 after each context switch between P1 and P2\n\n");
    
    /* Initialize the test */
    starvation_q1_init();
    
    /* Let the system run for a while */
    sleep(10);
    
    kprintf("\n*** Starvation Test Q1 Complete ***\n");
    return OK;
}

/*------------------------------------------------------------------------
 *  run_starvation_test_q2  -  Run starvation test for Q2 (time based)
 *------------------------------------------------------------------------
 */
process run_starvation_test_q2(void)
{
    kprintf("\n*** Starting Starvation Test Q2 (Time Based) ***\n\n");
    kprintf("Creating processes P1 (priority 40), P2 (priority 35), and Pstarv (priority 25)\n");
    kprintf("Pstarv's priority will be increased by 5 every 2 seconds it spends in the ready queue\n\n");
    
    /* Initialize the test */
    starvation_q2_init();
    
    /* Let the system run for a while */
    sleep(20);
    
    kprintf("\n*** Starvation Test Q2 Complete ***\n");
    return OK;
}
