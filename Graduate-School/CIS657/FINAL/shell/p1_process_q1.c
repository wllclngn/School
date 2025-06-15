/* p1_process_q1.c - Modified for XINU Final Project
 * Last modified: 2025-06-15 05:08:02 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <stdio.h>

void p1_func_q1(void) {
    int iterations = 0;
    int max_iterations = 50;

    while (iterations < max_iterations) {
        kprintf("[P1] PID=%d Priority=%d Iteration=%d\n",
                currpid, proctab[currpid].prprio, iterations + 1);
        iterations++;
        
        // Simulate some work
        volatile int j;
        for(j = 0; j < 10000; j++);
        
        // Give up CPU
        yield();
    }
}