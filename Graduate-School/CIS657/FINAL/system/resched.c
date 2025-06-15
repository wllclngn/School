/* resched.c - Modified for XINU Final Project
 * Last modified: 2025-06-15 06:42:05 UTC
 * Modified by: wllclngn
 */

#include <xinu.h>
#include <pstarv.h>

void boost_pstarv_priority(void) {
    if (enable_starvation_fix && pstarv_pid != BADPID) {
        struct procent *pstarv = &proctab[pstarv_pid];
        
        if (pstarv->prstate == PR_READY && pstarv_pid != currpid) {
            int old_prio = pstarv->prprio;
            pstarv->prprio = (pstarv->prprio + 2 > 50) ? 50 : pstarv->prprio + 2;
            
            kprintf("\nBOOST: PStarv priority increased from %d to %d\n", 
                    old_prio, pstarv->prprio);
            
            // Force preemption after boost if PStarv's new priority is higher
            if (pstarv->prprio > proctab[currpid].prprio) {
                resched();
            }
        }
    }
}

void resched(void) {
    struct procent *ptold;
    struct procent *ptnew;

    if (Defer.ndefers > 0) {
        Defer.attempt = TRUE;
        return;
    }

    ptold = &proctab[currpid];

    // Force rescheduling after each quantum
    if (ptold->prstate == PR_CURR) {
        ptold->prstate = PR_READY;
        insert(currpid, readylist, ptold->prprio);
        boost_pstarv_priority();  // Check for boost before selecting next process
    }

    currpid = dequeue(readylist);
    ptnew = &proctab[currpid];
    ptnew->prstate = PR_CURR;

    ctxsw(&ptold->prstkptr, &ptnew->prstkptr);
    return;
}