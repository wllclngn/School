#include <xinu.h>
#include <stdio.h>

#ifndef BADPID
#define BADPID (-1)
#endif

#ifndef CLKTICKS_PER_SEC
#define CLKTICKS_PER_SEC 1000
#endif

extern int enable_starvation_fix;
extern int pstarv_pid;

extern uint32 pstarv_ready_time;
extern uint32 last_boost_time;

void resched(void) {
    struct procent *ptold_proc;
    struct procent *ptnew_proc;
    pid32 old_pid;

    if (Defer.ndefers > 0) {
        Defer.attempt = TRUE;
        return;
    }

    ptold_proc = &proctab[currpid];
    old_pid = currpid;

    if (ptold_proc->prstate == PR_CURR) {
        ptold_proc->prstate = PR_READY;
        insert(currpid, readylist, ptold_proc->prprio);
    }

    currpid = dequeue(readylist);
    ptnew_proc = &proctab[currpid];
    ptnew_proc->prstate = PR_CURR;

#ifdef XINUSIM
    kprintf("Context switch: Old_PID=%d (%s, Prio:%d) ==> New_PID=%d (%s, Prio:%d)\n",
            old_pid, ptold_proc->prname, ptold_proc->prprio,
            currpid, ptnew_proc->prname, ptnew_proc->prprio);
#endif

    /* STARVATION FIX LOGIC START */
    if (pstarv_pid != BADPID && pstarv_pid < NPROC && proctab[pstarv_pid].prstate != PR_FREE) {
        struct procent *pstarv_entry = &proctab[pstarv_pid];
        
        if (enable_starvation_fix == TRUE) {
            /**************** Q1 LOGIC: Context-switch based boost ****************/
            if (pstarv_entry->prstate == PR_READY && old_pid != pstarv_pid) {
                if (pstarv_entry->prprio < MAXKEY) {
                    pri16 old_prio = pstarv_entry->prprio;
                    pstarv_entry->prprio += 1;

                    kprintf("DEBUG: Q1 Pstarv (PID: %d) priority incremented from %d to %d\n", pstarv_pid, old_prio, pstarv_entry->prprio);

                    /* Remove from ready list, then re-insert */
                    dequeue(pstarv_pid);
                    insert(pstarv_pid, readylist, pstarv_entry->prprio);
                    kprintf("DEBUG: Q1 Pstarv reinserted into ready queue with priority %d\n", pstarv_entry->prprio);
                }
            }
        } else {
            /**************** Q2 LOGIC: Time-based boost ****************/
            if (pstarv_entry->prstate == PR_READY) {
                uint32 current_time = clktime;
                uint32 time_since_last_boost = current_time - last_boost_time;

                if (time_since_last_boost >= (2 * CLKTICKS_PER_SEC)) {
                    pri16 old_prio_val = pstarv_entry->prprio;
                    pri16 new_prio_val = old_prio_val + 2;
                    const pri16 priority_cap = 42;

                    if (new_prio_val > priority_cap) {
                        new_prio_val = priority_cap;
                    }

                    if (new_prio_val > old_prio_val) {
                        pstarv_entry->prprio = new_prio_val;

                        /* Remove from ready list, then re-insert */
                        dequeue(pstarv_pid);
                        insert(pstarv_pid, readylist, pstarv_entry->prprio);

                        last_boost_time = current_time;

                        kprintf("DEBUG: Q2 Pstarv (PID: %d) priority boosted from %d to %d after %dms\n",
                                pstarv_pid, old_prio_val, new_prio_val, time_since_last_boost);
                    }
                }
            }

            /* pstarv_ready_time is now set in the timer interrupt handler */

        }
    }
    /* STARVATION FIX LOGIC END */

    ctxsw(&ptold_proc->prstkptr, &ptnew_proc->prstkptr);

    return;
}