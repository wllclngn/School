/* pstarv_process_q1.c - Modified for XINU Final Project
 * Last modified: 2025-06-15 05:19:17 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

void pstarv_func_q1(void) {
    int iterations = 0;
    int max_iterations = 50;
    int has_announced = 0;  // Using int instead of bool

    while (iterations < max_iterations) {
        // First time PStarv gets to run, show celebration
        if (!has_announced) {
            kprintf("\n===============================================\n");
            kprintf("PStarv (PID=%d) FINALLY RUNNING!\n", currpid);
            kprintf("Priority boosted to: %d\n", proctab[currpid].prprio);
            kprintf("wllclngn's starvation prevention works!\n");
            kprintf("===============================================\n\n");
            has_announced = 1;  // Using 1 instead of TRUE
        }

        kprintf("[PStarv] PID=%d Priority=%d Iteration=%d\n",
                currpid, proctab[currpid].prprio, iterations + 1);
        iterations++;
        
        // Simulate some work
        volatile int j;
        for(j = 0; j < 10000; j++);
        
        // Give up CPU
        yield();
    }
}