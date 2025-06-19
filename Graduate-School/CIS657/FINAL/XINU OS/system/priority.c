/* priority.c - chprio, getprio */

#include <xinu.h>

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
