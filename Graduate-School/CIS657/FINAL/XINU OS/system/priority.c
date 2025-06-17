/* priority.c - chprio, getprio, updatepriostarv */

#include <xinu.h>

/*------------------------------------------------------------------------
 * updatepriostarv - Update the priority of a starving process
 *------------------------------------------------------------------------
 */
syscall updatepriostarv(
    pid32   pid,            /* ID of process to change     */
    pri16   newprio         /* new priority                */
)
{
    intmask mask;           /* saved interrupt mask        */
    struct  procent *prptr; /* ptr to process' table entry */
    pri16   oldprio;        /* process's priority to return*/

    mask = disable();
    if (isbadpid(pid)) {
        restore(mask);
        return SYSERR;
    }
    
    prptr = &proctab[pid];
    oldprio = prptr->prprio;
    
    // Only update priority if the process is in a ready state
    // and is not currently running
    if ((prptr->prstate == PR_READY) && (pid != currpid)) {
        prptr->prprio = newprio;
        
        // Print message showing priority increase
        kprintf("Process %d ('Pstarv') priority increased to %d\n", pid, newprio);
        
        // Update the process in the ready list
        if (newprio > oldprio) {
            // Remove from the ready list
            getitem(pid);
            
            // Insert with the new priority
            insert(pid, readylist, newprio);
        }
    }
    
    restore(mask);
    return oldprio;
}

/*------------------------------------------------------------------------
 * chprio  -  Change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
pri16   chprio(
    pid32   pid,            /* ID of process to change     */
    pri16   newprio         /* new priority                */
    )
{
    intmask mask;           /* saved interrupt mask        */
    struct  procent *prptr; /* ptr to process' table entry */
    pri16   oldprio;        /* process's priority to return*/

    mask = disable();
    if (isbadpid(pid)) {
        restore(mask);
        return (pri16)SYSERR;
    }
    prptr = &proctab[pid];
    oldprio = prptr->prprio;
    prptr->prprio = newprio;
    restore(mask);
    return oldprio;
}

/*------------------------------------------------------------------------
 *  getprio  -  Return the scheduling priority of a given process
 *------------------------------------------------------------------------
 */
pri16   getprio(
    pid32   pid             /* ID of process to use        */
    )
{
    intmask mask;           /* saved interrupt mask        */
    struct  procent *prptr; /* ptr to process' table entry */
    pri16   prio;           /* priority to return          */

    mask = disable();
    if (isbadpid(pid)) {
        restore(mask);
        return (pri16)SYSERR;
    }
    prptr = &proctab[pid];
    prio = prptr->prprio;
    restore(mask);
    return prio;
}
