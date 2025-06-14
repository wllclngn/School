#include <xinu.h>

// Ensure these are correctly defined or included from pstarv.h
#ifndef BADPID
#define BADPID          (-1)
#endif

#ifndef CLKTICKS_PER_SEC
#define CLKTICKS_PER_SEC 1000
#endif

// Global variables for starvation fix (ensure these are defined elsewhere, e.g., initialize.c or pstarv.c)
extern int enable_starvation_fix; // TRUE for Q1 (context-switch based), FALSE for Q2 (time-based)
extern int pstarv_pid;            // PID of the process to monitor for starvation

// Q2 specific variables (ensure these are defined if Q2 logic is active)
extern uint32 pstarv_ready_time; // Time when Pstarv entered ready queue (for Q2)
extern uint32 last_boost_time;   // Time of last priority boost for Pstarv (for Q2)


void resched(void) {
    struct procent *ptold_proc;  /* Pointer to the old process table entry */
    struct procent *ptnew_proc;  /* Pointer to the new process table entry */
    pid32 old_pid;               /* PID of the process being switched out */

    /* If rescheduling is deferred, record attempt and return */
    if (Defer.ndefers > 0) {
        Defer.attempt = TRUE;
        return;
    }

    /* Point to process table entry for the current (old) process */
    ptold_proc = &proctab[currpid];
    old_pid = currpid; // Save the PID of the process being switched out

    /* If the current process was running, place it on the ready list */
    if (ptold_proc->prstate == PR_CURR) {
        ptold_proc->prstate = PR_READY;
        insert(currpid, readylist, ptold_proc->prprio);
    }

    /* Force context switch to highest priority ready process */
    currpid = dequeue(readylist); // currpid is NOW the NEW process ID
    ptnew_proc = &proctab[currpid];
    ptnew_proc->prstate = PR_CURR;

#ifdef XINUSIM
    kprintf("Context switch: Old_PID=%d (%s, Prio:%d) ==> New_PID=%d (%s, Prio:%d)\n",
            old_pid, ptold_proc->prname, ptold_proc->prprio,
            currpid, ptnew_proc->prname, ptnew_proc->prprio);
#endif

    /* STARVATION FIX LOGIC START */
    if (pstarv_pid != BADPID) { // Only proceed if we have a valid Pstarv PID
        struct procent *pstarv_entry = &proctab[pstarv_pid];

        if (enable_starvation_fix == TRUE) {
            /**************** Q1 LOGIC: Context-switch based boost ****************/
            // Boost Pstarv's priority if:
            // 1. Pstarv is in the ready queue (waiting to run).
            // 2. The process being switched OUT (old_pid) was NOT Pstarv itself.
            if (pstarv_entry->prstate == PR_READY && old_pid != pstarv_pid) {
                kprintf("DEBUG RESCHED Q1: CTXSW OldPID=%d (%s) -> NewPID=%d (%s). Pstarv(PID:%d) is PR_READY.\n",
                        old_pid, ptold_proc->prname, currpid, ptnew_proc->prname, pstarv_pid);
                kprintf("DEBUG RESCHED Q1: Attempting to boost Pstarv (PID: %d). Current Prio: %d.\n",
                        pstarv_pid, pstarv_entry->prprio);

                if (pstarv_entry->prprio < MAXKEY) { // MAXKEY is usually the max priority
                    pstarv_entry->prprio += 1; 
                    kprintf("DEBUG RESCHED Q1: Pstarv (PID: %d) NEW Prio: %d.\n",
                            pstarv_pid, pstarv_entry->prprio);

                    getitem(pstarv_pid); 
                    insert(pstarv_pid, readylist, pstarv_entry->prprio);

                } else {
                    kprintf("DEBUG RESCHED Q1: Pstarv (PID: %d) already at max Prio: %d.\n",
                            pstarv_pid, pstarv_entry->prprio);
                }
            } else if (pstarv_entry->prstate != PR_READY) {
                kprintf("DEBUG RESCHED Q1: Pstarv (PID: %d) is not in PR_READY state (state: %d), not boosting for Q1.\n", pstarv_pid, pstarv_entry->prstate);
            } else if (old_pid == pstarv_pid) {
                kprintf("DEBUG RESCHED Q1: Context switch is FROM Pstarv (PID: %d was old_pid), not boosting this cycle for Q1.\n", pstarv_pid);
            }
        } else {
            /**************** Q2 LOGIC: Time-based boost ****************/
            if (pstarv_entry->prstate == PR_READY) {
                uint32 current_time = clktime; 
                uint32 time_in_ready_queue = current_time - pstarv_ready_time; 
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
                        
                        getitem(pstarv_pid);
                        insert(pstarv_pid, readylist, pstarv_entry->prprio);
                        
                        kprintf("DEBUG RESCHED Q2: Pstarv (PID: %d) priority %d -> %d after %ums in ready (actual time %ums since last boost). Time: %d\n",
                                pstarv_pid, old_prio_val, new_prio_val, 
                                time_in_ready_queue, 
                                time_since_last_boost, 
                                current_time);
                        last_boost_time = current_time; 
                    }
                }
            }
            
            if (pstarv_pid == currpid) { // Pstarv is NOW the current process (the one being switched TO)
                if (pstarv_ready_time != 0) { 
                    kprintf("DEBUG RESCHED Q2: Pstarv (PID: %d) is now running. Resetting Q2 ready timers. Old pstarv_ready_time: %d, Old last_boost_time: %d\n", pstarv_pid, pstarv_ready_time, last_boost_time);
                    pstarv_ready_time = 0; 
                    last_boost_time = clktime; 
                }
            } else if (pstarv_entry->prstate == PR_READY && pstarv_ready_time == 0) {
                // Pstarv just entered the ready queue OR is found ready and wasn't being timed for Q2
                kprintf("DEBUG RESCHED Q2: Pstarv (PID: %d) detected in PR_READY. Initializing Q2 timers. Current time: %d\n", pstarv_pid, clktime);
                pstarv_ready_time = clktime;
                last_boost_time = clktime; 
            }
        }
    }
    /* STARVATION FIX LOGIC END */

    /* Perform context switch to new process */
    ctxsw(&ptold_proc->prstkptr, &ptnew_proc->prstkptr);

    return;
}