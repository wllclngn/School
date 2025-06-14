#include <xinu.h>

#ifndef BADPID
/* invalid process ID */
#define BADPID          (-1)
#endif

#ifndef CLKTICKS_PER_SEC
#define CLKTICKS_PER_SEC 1000   /* clock ticks per second (adjust if needed) */
#endif

/* Starvation fix variables - define locally if pstarv.h doesn't exist */
extern int enable_starvation_fix;
extern int pstarv_pid;
extern uint32 pstarv_ready_time;
extern uint32 last_boost_time;

void resched(void)
{
    struct procent *ptold;  /* ptr to old process entry */
    struct procent *ptnew;  /* ptr to new process entry */

    /* If rescheduling is deferred, record attempt and return */
    if (Defer.ndefers > 0) {
        Defer.attempt = TRUE;
        return;
    }

    /* Point to process table entry for the current (old) process */
    ptold = &proctab[currpid];

    /* If the current process is running, place it on ready list */
    if (ptold->prstate == PR_CURR) {
        ptold->prstate = PR_READY;
        insert(currpid, readylist, ptold->prprio);
    }

    /* Force context switch to highest priority ready process */
    currpid = dequeue(readylist);
    ptnew = &proctab[currpid];
    ptnew->prstate = PR_CURR;

    /* Debug output for simulation mode (if XINUSIM is defined) */
#ifdef XINUSIM
    kprintf("Context switch: Old_PID=%d (%s, Prio:%d) ==> New_PID=%d (%s, Prio:%d)\n",
            (ptold - proctab), ptold->prname, ptold->prprio,
            currpid, ptnew->prname, ptnew->prprio);
#endif

    /* STARVATION FIX LOGIC START */
    /* Only run if starvation fix is enabled and we have a valid pstarv_pid */
    if (enable_starvation_fix && pstarv_pid != BADPID) {
        struct procent *starv_proc_entry = &proctab[pstarv_pid];

        if (starv_proc_entry->prstate == PR_READY) {
            /* Determine which mode to use based on pstarv_ready_time */
            if (pstarv_ready_time == 0) {
                /* Q1 MODE: Context-switch-based (pstarv_ready_time not set) */
                if (pstarv_pid != currpid) {
                    pri16 old_prio = starv_proc_entry->prprio;
                    pri16 new_prio = old_prio + 2;
                    const pri16 priority_cap = 42;

                    if (new_prio > priority_cap) {
                        new_prio = priority_cap;
                    }

                    if (new_prio > old_prio) {
                        starv_proc_entry->prprio = new_prio;
                        /* Try to update priority on ready list */
                        if (getitem(pstarv_pid) != SYSERR) {
                            insert(pstarv_pid, readylist, new_prio);
                            kprintf("Starvation Fix: Pstarv (PID: %d) priority boosted from %d to %d. (CS: %s -> %s)\n",
                                     pstarv_pid, old_prio, new_prio, ptold->prname, ptnew->prname);
                        } else {
                            kprintf("Starvation Fix WARNING: Pstarv (PID: %d, State: %d) was PR_READY but not found on readylist. Prio updated to %d.\n",
                                     pstarv_pid, starv_proc_entry->prstate, new_prio);
                        }
                    }
                }
            } else {
                /* Q2 MODE: Time-based (pstarv_ready_time is set) */
                uint32 current_time = clktime;
                uint32 time_in_ready = current_time - pstarv_ready_time;
                uint32 time_since_boost = current_time - last_boost_time;

                if (time_since_boost >= (2 * CLKTICKS_PER_SEC)) {
                    pri16 old_prio = starv_proc_entry->prprio;
                    pri16 new_prio = old_prio + 2;
                    const pri16 priority_cap = 42;

                    if (new_prio > priority_cap) {
                        new_prio = priority_cap;
                    }

                    if (new_prio > old_prio) {
                        starv_proc_entry->prprio = new_prio;
                        /* Try to update priority on ready list */
                        if (getitem(pstarv_pid) != SYSERR) {
                            insert(pstarv_pid, readylist, new_prio);
                            kprintf("TIME-BASED BOOST: Pstarv (PID: %d) priority %d -> %d after %d seconds in ready queue (time: %d)\n",
                                    pstarv_pid, old_prio, new_prio, time_in_ready / CLKTICKS_PER_SEC, current_time);
                            last_boost_time = current_time;
                        }
                    }
                }
            }
        } else if (starv_proc_entry->prstate == PR_CURR && pstarv_pid == currpid) {
            /* Reset time tracking when Pstarv starts running (Q2 mode only) */
            if (pstarv_ready_time != 0) {
                pstarv_ready_time = 0;
                last_boost_time = 0;
            }
        }

        /* Q2 MODE: Initialize time tracking when Pstarv first enters ready queue */
        if (starv_proc_entry->prstate == PR_READY && pstarv_ready_time == 0 && last_boost_time != 0) {
            pstarv_ready_time = clktime;
            last_boost_time = clktime;
            kprintf("Pstarv entered ready queue at time %d\n", clktime);
        }
    }
    /* STARVATION FIX LOGIC END */

    /* Perform context switch to new process */
    ctxsw(&ptold->prstkptr, &ptnew->prstkptr);

    return;
}