/* starvation_prevention.c - Starvation prevention mechanisms for XINU Final Project */

#include <xinu.h>
#include <pstarv.h>

/*------------------------------------------------------------------------
 * updatepriostarv - Helper function to update the priority of a process
 *------------------------------------------------------------------------
 */
syscall updatepriostarv(pid32 pid, pri16 newprio)
{
    intmask mask;                   /* Saved interrupt mask */
    struct procent *prptr;          /* Pointer to process table entry */
    pri16 oldprio;                  /* Previous priority value */

    mask = disable();
    if (isbadpid(pid)) {
        restore(mask);
        return SYSERR;
    }

    prptr = &proctab[pid];
    oldprio = prptr->prprio;
    prptr->prprio = newprio;

    /* If process is in the ready list, remove and reinsert it with new priority */
    if (prptr->prstate == PR_READY) {
        getitem(pid);               /* Remove from ready list */
        insert(pid, readylist, newprio);  /* Reinsert with new priority */
    }

    restore(mask);
    return oldprio;    /* Return old priority value for consistency */
}

/*------------------------------------------------------------------------
 * boost_pstarv_priority - Boost the priority of the starving process
 *------------------------------------------------------------------------
 */
void boost_pstarv_priority(void)
{
    intmask mask;                   /* Saved interrupt mask */
    struct procent *prptr;          /* Pointer to process table entry */
    
    mask = disable();
    
    if (starvingPID != BADPID) {
        prptr = &proctab[starvingPID];
        if (prptr->prstate != PR_FREE) {
            pri16 oldprio = prptr->prprio;
            pri16 newprio = (oldprio + 2 > MAXPRIO) ? MAXPRIO : oldprio + 2; /* Increase by 2, up to max */
            
            updatepriostarv(starvingPID, newprio);
            
            kprintf("BOOST: Pstarv (PID: %d) priority increased from %d to %d\n", 
                    starvingPID, oldprio, newprio);
                    
            /* Update last boost time */
            last_boost_time = clktime;
        }
    }
    
    restore(mask);
}

/*------------------------------------------------------------------------
 * check_pstarv_time - Check if Pstarv has been in ready queue too long
 *------------------------------------------------------------------------
 */
void check_pstarv_time(void)
{
    /* Check if starvation prevention is enabled */
    if (!enable_starvation_fix) {
        return;
    }
    
    intmask mask;                   /* Saved interrupt mask */
    uint32 current_time;
    uint32 time_in_ready;
    struct procent *prptr;
    
    mask = disable();
    
    if (pstarv_pid != BADPID) {
        prptr = &proctab[pstarv_pid];
        
        if (prptr->prstate == PR_READY) {
            current_time = clktime;
            time_in_ready = current_time - pstarv_ready_time;
            
            /* Check if 2 seconds have passed since last priority boost */
            if (time_in_ready >= 2) {
                pri16 oldprio = prptr->prprio;
                pri16 newprio = (oldprio + 5 > MAXPRIO) ? MAXPRIO : oldprio + 5; /* Increase by 5, up to max */
                
                updatepriostarv(pstarv_pid, newprio);
                
                kprintf("TIME-BOOST: Pstarv (PID: %d) priority increased from %d to %d after %d seconds\n",
                        pstarv_pid, oldprio, newprio, time_in_ready);
                        
                /* Reset the ready time */
                pstarv_ready_time = current_time;
            }
        }
    }
    
    restore(mask);
}
