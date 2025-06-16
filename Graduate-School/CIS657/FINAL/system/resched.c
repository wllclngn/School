#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  resched  -  Reschedule processor to highest priority eligible process
 *------------------------------------------------------------------------
 */
void resched(void)
{
    struct procent *ptold; /* Ptr to process being replaced */
    struct procent *ptnew; /* Ptr to highest priority process to run */

    /* If rescheduling is deferred, record attempt to reschedule */

    if (Defer.ndefers > 0) {
        Defer.attempt = TRUE;
        return;
    }

    /* Point to per-process table entry for the current process */

    ptold = &proctab[currpid];

    /* Check for starving processes and update their priorities */
    if (starvation_prevention && starvingPID != 0) {
        pri16 curr_prio = getprio(starvingPID);
        if (curr_prio < MAXPRIO) {
            /* Increase priority by 2 */
            pri16 new_prio = curr_prio + 2;
            chprio(starvingPID, new_prio);

            /* Print the priority boost information */
            kprintf("BOOST: PStarv (PID: %d) priority increased from %d to %d\n",
                    starvingPID, curr_prio, new_prio);
        }
    }

    /* If current process has a higher priority than what is in the */
    /* ready list, then continue to run the current process */

    if ((ptold->prstate == PR_CURR) &&
        (ptold->prprio > firstkey(readylist))) {
        return;
    }

    /* Force context switch to highest priority ready process */

    if (ptold->prstate == PR_CURR) {
        /* Place current process back on ready list */
        ptold->prstate = PR_READY;
        insert(currpid, readylist, ptold->prprio);
    }

    /* Remove process of highest priority from ready queue */

    currpid = dequeue(readylist);
    ptnew = &proctab[currpid];
    ptnew->prstate = PR_CURR; /* Mark it currently running */

#ifdef RTCLOCK
    preempt = QUANTUM;        /* Reset preemption counter */
#endif

    /* Context switch to next ready process */

    ctxsw(&ptold->prstkptr, &ptnew->prstkptr);

    /* Old process returns here when resumed */

    return;
}

/*------------------------------------------------------------------------
 *  resched_cntl  -  Control whether rescheduling is deferred
 *------------------------------------------------------------------------
 */
unsigned long resched_cntl(int defer)
{
    unsigned long mask; /* Saved interrupt mask */

    mask = Defer.ndefers;

    /* Handle startup of deferred rescheduling */

    if (defer == DEFER_START) {
        /* Disable interrupts */

        /* Increment number of deferrals */

        Defer.ndefers++;

        /* Restore interrupts */

        return mask;
    }

    /* Handle ending of deferred rescheduling */

    if (defer == DEFER_STOP) {
        /* Disable interrupts */

        /* Decrement number of deferrals */

        if (Defer.ndefers > 0) {
            Defer.ndefers--;
        }

        /* Reschedule if necessary */

        if ((Defer.ndefers == 0) && Defer.attempt) {
            Defer.attempt = FALSE;
            resched();
        }

        /* Restore interrupts */

        return mask;
    }

    return mask;
}