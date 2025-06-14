#include <xinu.h>

#ifndef BADPID
#define BADPID          (-1)
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

/*
#ifdef XINUSIM
    kprintf("Context switch: Old_PID=%d (%s, Prio:%d) ==> New_PID=%d (%s, Prio:%d)\n",
            old_pid, ptold_proc->prname, ptold_proc->prprio,
            currpid, ptnew_proc->prname, ptnew_proc->prprio);
#endif
*/

    /* STARVATION FIX LOGIC START */
    if (pstarv_pid != BADPID && pstarv_pid < NPROC && proctab[pstarv_pid].prstate != PR_FREE) {
        struct procent *pstarv_entry = &proctab[pstarv_pid];

        if (enable_starvation_fix == TRUE) {
            /**************** Q1 LOGIC: Context-switch based boost ****************/
            if (pstarv_entry->prstate == PR_READY && old_pid != pstarv_pid) {
                if (pstarv_entry->prprio < MAXKEY) {
                    pstarv_entry->prprio += 1;
                    if (getitem(pstarv_pid) != SYSERR) {
                         insert(pstarv_pid, readylist, pstarv_entry->prprio);
                    }
                }
            }
            /*
            else if (pstarv_entry->prstate != PR_READY) {
                // This kprintf was causing issues or spam, keep it commented/removed for cleaner version
                // kprintf("DEBUG RESCHED Q1: Pstarv (PID: %d) is not in PR_READY state (state: %d), not boosting for Q1.\\n\", pstarv_pid, pstarv_entry->prstate);
            }
            */
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
                        if (getitem(pstarv_pid) != SYSERR) {
                            insert(pstarv_pid, readylist, pstarv_entry->prprio);
                        }
                        last_boost_time = current_time;
                    }
                }
            }

            if (pstarv_pid == currpid) {
                if (pstarv_ready_time != 0) {
                    pstarv_ready_time = 0;
                    last_boost_time = clktime;
                }
            } else if (pstarv_entry->prstate == PR_READY && pstarv_ready_time == 0) {
                pstarv_ready_time = clktime;
                last_boost_time = clktime;
            }
        }
    }
    /* STARVATION FIX LOGIC END */

    ctxsw(&ptold_proc->prstkptr, &ptnew_proc->prstkptr);

    return;
}