/* resched.c - Final Version for Tonight
 * Last modified: 2025-06-15 15:54:16 UTC
 * Modified by: wllclngn
 *
 * LAST ATTEMPT: If this doesn't work, we'll tackle it
 * after some much-needed rest.
 */

#include <xinu.h>
#include <pstarv.h>

// Global round-robin counter
static unsigned int rr_count = 0;

void boost_pstarv_priority(void) {
    if (enable_starvation_fix && pstarv_pid != BADPID) {
        struct procent *pstarv = &proctab[pstarv_pid];
        if (pstarv->prstate == PR_READY) {
            // Increment priority, but don't exceed maximum
            pstarv->prprio = min(pstarv->prprio + 2, 50);
            kprintf("BOOST: PStarv priority increased from %d to %d\n",
                    pstarv->prprio - 2, pstarv->prprio);
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

    /* Absolute simplest approach possible:
     * 1. ALWAYS put current process back in ready queue
     * 2. ALWAYS pick next process from ready queue
     * 3. Every context switch, boost PStarv's priority
     */
    if (ptold->prstate == PR_CURR) {
        ptold->prstate = PR_READY;
        insert(currpid, readylist, ptold->prprio);

        boost_pstarv_priority();  // Boost on EVERY context switch
    }

    currpid = dequeue(readylist);
    ptnew = &proctab[currpid];
    ptnew->prstate = PR_CURR;

    ctxsw(&ptold->prstkptr, &ptnew->prstkptr);
    return;
}