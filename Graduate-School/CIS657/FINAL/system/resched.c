#include <xinu.h>

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

    /* Handle current process - CRITICAL CHANGE HERE */
    if (ptold_proc->prstate == PR_CURR) {
        if (currpid != NULLPROC) {
            ptold_proc->prstate = PR_READY;
            insert(currpid, readylist, ptold_proc->prprio);
        }
    }

    /* Starvation Prevention - BEFORE selecting next process */
    if (enable_starvation_fix && 
        pstarv_pid != BADPID && 
        pstarv_pid < NPROC && 
        proctab[pstarv_pid].prstate == PR_READY) {
        
        struct procent *pstarv_entry = &proctab[pstarv_pid];
        
        /* Remove from current position if in ready queue */
        if (getitem(pstarv_pid) != SYSERR) {
            /* Boost priority */
            if (pstarv_entry->prprio < MAXKEY - 2) {
                pstarv_entry->prprio += 2;
                kprintf("DEBUG: Pstarv priority boosted to %d\n", pstarv_entry->prprio);
            }
            
            /* Reinsert with new priority */
            insert(pstarv_pid, readylist, pstarv_entry->prprio);
        }
    }

    /* Select highest priority process */
    pid32 next_pid = dequeue(readylist);
    
    /* CRITICAL CHANGE - Proper handling when ready queue is empty */
    if (next_pid == EMPTY || next_pid == NULLPROC) {
        /* If no other process is ready, run NULLPROC */
        currpid = NULLPROC;
        ptnew_proc = &proctab[NULLPROC];
        ptnew_proc->prstate = PR_CURR;
    } else {
        currpid = next_pid;
        ptnew_proc = &proctab[currpid];
        ptnew_proc->prstate = PR_CURR;
    }

    /* Debug output for context switch */
    kprintf("Context switch: Old_PID=%d (%s, Prio:%d) ==> New_PID=%d (%s, Prio:%d)\n",
            old_pid, ptold_proc->prname, ptold_proc->prprio,
            currpid, ptnew_proc->prname, ptnew_proc->prprio);

    /* Perform context switch */
    ctxsw(&ptold_proc->prstkptr, &ptnew_proc->prstkptr);
}