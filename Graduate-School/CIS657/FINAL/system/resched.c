#include <xinu.h>

#ifndef BADPID
#define BADPID (-1)
#endif

#ifndef CLKTICKS_PER_SEC
#define CLKTICKS_PER_SEC 1000 // Should be in clock.h
#endif

#ifndef MAXKEY // Ensure MAXKEY is defined, e.g. in process.h or queue.h
#define MAXKEY 255 // A common high priority value, adjust if your system uses a different max
#endif

// Global variables for starvation fix (declared in externs.h, defined in initialize.c)
extern int g_enable_starvation_fix;
extern pid32 g_pstarv_pid;
extern uint32 g_pstarv_ready_time;
extern uint32 g_last_boost_time;

void resched(void) {
    struct procent *ptold_proc; // Pointer to old process entry
    struct procent *ptnew_proc; // Pointer to new process entry
    pid32 old_pid;              // PID of the old process

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

    // kprintf("CS: %d (%s pri:%d) -> %d (%s pri:%d)\n",
    //         old_pid, ptold_proc->prname, ptold_proc->prprio,
    //         currpid, ptnew_proc->prname, ptnew_proc->prprio);

    /* STARVATION FIX LOGIC START */
    // Check if Pstarv is a valid process and not free
    if (g_pstarv_pid != BADPID && g_pstarv_pid < NPROC && proctab[g_pstarv_pid].prstate != PR_FREE) {
        struct procent *pstarv_entry = &proctab[g_pstarv_pid];
        
        if (g_enable_starvation_fix == TRUE) {
            /**************** Q1 LOGIC: Context-switch based boost ****************/
            // Boost only if Pstarv is ready, and the context switch is between other processes
            // (i.e., neither old nor new is Pstarv, and old is not the same as new)
            if (pstarv_entry->prstate == PR_READY && old_pid != g_pstarv_pid && currpid != g_pstarv_pid && old_pid != currpid) {
                pid32 temp_pstarv_check = BADPID;

                // Attempt to remove Pstarv from readylist before updating priority
                temp_pstarv_check = getitem(g_pstarv_pid);

                if (pstarv_entry->prprio < MAXKEY) {
                    pstarv_entry->prprio += 1; // Increment priority by 1
                    kprintf("[RESCHED-Q1] Pstarv (PID:%d) priority boosted to %d\n", g_pstarv_pid, pstarv_entry->prprio);
                }

                if (temp_pstarv_check == g_pstarv_pid) { // If it was successfully removed
                    insert(g_pstarv_pid, readylist, pstarv_entry->prprio);
                } else if (temp_pstarv_check == SYSERR && pstarv_entry->prstate == PR_READY) {
                    // Was PR_READY but getitem failed (e.g. not in readylist). Insert it.
                    insert(g_pstarv_pid, readylist, pstarv_entry->prprio);
                     kprintf("[RESCHED-Q1 WARN] Pstarv (PID:%d) was PR_READY but not in list (or getitem failed), inserted with prio %d\n", g_pstarv_pid, pstarv_entry->prprio);
                }
            }
        } else {
            /**************** Q2 LOGIC: Time-based boost ****************/
            // If Pstarv is the one currently running, reset its ready timer and last boost time
            if (currpid == g_pstarv_pid) {
                g_pstarv_ready_time = 0; // Reset: it's no longer just "ready", it's "running"
                g_last_boost_time = clktime; // Update last boost time as it got to run
            }
            // If Pstarv is ready but not running
            else if (pstarv_entry->prstate == PR_READY) {
                uint32 current_time_ticks = clktime;

                // If this is the first time we see Pstarv as ready since it last ran (or started)
                if (g_pstarv_ready_time == 0) {
                     g_pstarv_ready_time = current_time_ticks;
                     // Initialize last_boost_time here as well, or ensure it's set when Pstarv becomes ready.
                     // If we set it here, it means the 2-second timer for boost starts now.
                     g_last_boost_time = current_time_ticks;
                }
                
                // Check if 2 seconds (2 * CLKTICKS_PER_SEC) have passed since Pstarv was last boosted
                // AND it has been in the ready queue.
                if ((current_time_ticks - g_last_boost_time) >= (2 * CLKTICKS_PER_SEC)) {
                    pid32 temp_pstarv_check_q2 = BADPID;
                    pri16 old_prio_val = pstarv_entry->prprio;
                    pri16 new_prio_val = old_prio_val + 2; // Increment by 2 for Q2
                    // Define a reasonable priority cap, could be MAXKEY or a bit less
                    const pri16 priority_cap = (MAXKEY < 60) ? MAXKEY : 60;


                    if (new_prio_val > priority_cap) {
                        new_prio_val = priority_cap;
                    }

                    if (new_prio_val > old_prio_val) { // Only if priority actually increases
                        temp_pstarv_check_q2 = getitem(g_pstarv_pid); // Remove from ready list
                        pstarv_entry->prprio = new_prio_val;          // Update proctab priority
                        
                        if (temp_pstarv_check_q2 == g_pstarv_pid) { // If successfully removed
                           insert(g_pstarv_pid, readylist, pstarv_entry->prprio); // Re-insert with new priority
                        } else if (temp_pstarv_check_q2 == SYSERR && pstarv_entry->prstate == PR_READY){
                           // Was PR_READY but getitem failed. Insert it.
                           insert(g_pstarv_pid, readylist, pstarv_entry->prprio);
                           kprintf("[RESCHED-Q2 WARN] Pstarv (PID:%d) was PR_READY but not in list (or getitem failed), inserted with prio %d\n", g_pstarv_pid, pstarv_entry->prprio);
                        }
                        
                        kprintf("[RESCHED-Q2] Pstarv (PID:%d) priority time-boosted to %d\n",
                                g_pstarv_pid, pstarv_entry->prprio);
                        g_last_boost_time = current_time_ticks; // Update last boost time
                        // g_pstarv_ready_time should remain to track total time in ready if needed,
                        // or reset if each boost "resets" its ready duration for the *next* boost interval.
                        // For "every two seconds the Pstarv process is in the ready queue",
                        // g_last_boost_time is the critical marker.
                    }
                }
            } else { // Pstarv is not PR_READY and not PR_CURR (e.g. PR_SLEEP, PR_SUSP)
                 g_pstarv_ready_time = 0; // Reset ready timer if it's no longer ready
            }
        }
    }
    /* STARVATION FIX LOGIC END */

    ctxsw(&ptold_proc->prstkptr, &ptnew_proc->prstkptr); // Perform context switch

    return;
}