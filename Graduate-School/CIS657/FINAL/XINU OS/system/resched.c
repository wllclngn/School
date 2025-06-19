/* resched.c - resched */

#include <xinu.h>
#include <pstarv.h>

/*------------------------------------------------------------------------
 * resched  -  Reschedule processor to highest priority eligible process
 *------------------------------------------------------------------------
 */
void	resched(void)
{
	struct procent *ptold;	/* Ptr to table entry for old process	*/
	struct procent *ptnew;	/* Ptr to table entry for new process	*/

	/* If rescheduling is deferred, record attempt and return */

	if (Defer.ndefers > 0) {
		Defer.attempt = TRUE;
		return;
	}

	/* Point to process table entry for the current (old) process */
	ptold = &proctab[currpid];

	/* Check for starvation prevention (Question 1) */
	if (starvation_prevention && starvingPID != BADPID) {
		struct procent *pstarv = &proctab[starvingPID];
		if (pstarv->prstate != PR_FREE) {
			pri16 old_prio = pstarv->prprio;
			if (old_prio < MAXPRIO) {
				pri16 new_prio = (old_prio + 2 > MAXPRIO) ? MAXPRIO : old_prio + 2;
				updatepriostarv(starvingPID, new_prio);
				
				kprintf("BOOST: PStarv (PID: %d) priority increased from %d to %d\n",
						starvingPID, old_prio, new_prio);
			}
		}
	}

	if (ptold->prstate == PR_CURR) {  /* Process remains eligible */
		if (ptold->prprio > firstkey(readylist)) {
			return;
		}

		/* Old process will no longer remain current */
		ptold->prstate = PR_READY;
		insert(currpid, readylist, ptold->prprio);
	}

	/* Force context switch to highest priority ready process */
	currpid = dequeue(readylist);
	ptnew = &proctab[currpid];
	ptnew->prstate = PR_CURR;
	preempt = QUANTUM;		/* Reset time slice for process	*/

	/* Log the context switch event */
	char *old_name = (ptold->prname[0] != '\0') ? ptold->prname : "unknown";
	char *new_name = (ptnew->prname[0] != '\0') ? ptnew->prname : "unknown";
	
	kprintf("CONTEXT SWITCH: From PID=%d (%s) to PID=%d (%s)\n",
			ptold->prpid, old_name, ptnew->prpid, new_name);
			
	/* For Question 2: Record when PStarv enters the ready state */
	if (ptnew->prpid == pstarv_pid && pstarv_pid != BADPID) {
		pstarv_ready_time = clktime;
		kprintf("READY TIME: PStarv entered ready state at time %d\n", pstarv_ready_time);
	}

	/* Context switch to next ready process */
	ctxsw(&ptold->prstkptr, &ptnew->prstkptr);

	/* Old process returns here when resumed */
	return;
}
