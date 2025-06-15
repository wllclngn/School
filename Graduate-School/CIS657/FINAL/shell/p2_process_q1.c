/* p2_process_q1.c - Modified for XINU Final Project
 * Last modified: 2025-06-15 05:24:02 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <stdio.h>

void p2_func_q1(void) {
    int count = 0;
    
    while (count < 20) {  // Simple counter-based loop
        kprintf("P2 (PID: %d, Priority: %d) iteration %d/20\n",
                currpid, proctab[currpid].prprio, count + 1);
        count++;
        yield();  // Give up CPU
    }
}