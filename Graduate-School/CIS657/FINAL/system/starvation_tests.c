/* starvation_tests.c - Command handlers for starvation tests */

#include <xinu.h>
#include <pstarv.h>

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
