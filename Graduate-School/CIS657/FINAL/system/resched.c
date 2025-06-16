#include <xinu.h>
#include <pstarv.h>
#include <stdio.h>

extern void p1_func(void);
extern void p2_func(void);

// New function for PStarv to check its ready time and boost priority
void check_pstarv_time(void) {
    uint32 current_time;
    uint32 time_in_ready_queue;

    if (enable_starvation_fix == FALSE && pstarv_pid != BADPID) {
        struct procent *pstarv = &proctab[pstarv_pid];
        if (pstarv->prstate == PR_READY) {
            current_time = clktime;
            time_in_ready_queue = current_time - pstarv_ready_time;

            if (time_in_ready_queue >= 2 * CLKTICKS_PER_SEC) {
                pri16 old_prio = pstarv->prprio;
                pstarv->prprio = min(pstarv->prprio + 5, 50); // Boost priority
                
                /* Print with nanosecond precision using clkticks for sub-second precision */
                uint32 nanos = ((time_in_ready_queue % CLKTICKS_PER_SEC) * 1000000000) / CLKTICKS_PER_SEC;
                kprintf("TIMEBOOST: PStarv priority increased from %d to %d after %d.%09d seconds\n",
                        old_prio, pstarv->prprio, 
                        time_in_ready_queue / CLKTICKS_PER_SEC,
                        nanos);
                        
                pstarv_ready_time = current_time; // Reset ready time
            }
        }
    }
}

void pstarv_func_q2(void) {
    kprintf("\n##########################################################################\n");
    kprintf("Pstarv (PID: %d, Prio: %d) HAS STARTED at time %d! Hooray!\n",
            currpid, proctab[currpid].prprio, clktime);
    kprintf("I (wllclngn) will get a good grade! Time-based scheduling works!\n");
    kprintf("##########################################################################\n\n");

    // Loop and check time periodically
    while (1) {
        check_pstarv_time();
        sleep(1); // Check every second
    }
}

// Q1 solution - context switch-based starvation prevention
shellcmd starvation_test1(int nargs, char *args[]) {
    pid32 p1_pid, p2_pid, pstarv_pid_local;

    if (nargs > 1) {
        kprintf("Usage: starvation_test1\n");
        return SHELL_ERROR;
    }

    kprintf("\n=====================================================\n");
    kprintf("QUESTION 1: CONTEXT SWITCH-BASED STARVATION PREVENTION\n");
    kprintf("=====================================================\n\n");

    kprintf("Starting context switch-based starvation simulation...\n");

    // Set up starvation prevention parameters
    starvation_prevention = TRUE;     // Q1: context switch-based boost is TRUE
    
    // Create processes with different priorities
    p1_pid = create(p1_func, 4096, 40, "P1_Process_Q1", 0);
    p2_pid = create(p2_func, 4096, 35, "P2_Process_Q1", 0);
    pstarv_pid_local = create(pstarv_func_q1, 4096, 25, "PStarv_Process_Q1", 0);

    if (p1_pid == SYSERR || p2_pid == SYSERR || pstarv_pid_local == SYSERR) {
        kprintf("Error: Failed to create one or more processes.\n");
        if (p1_pid != SYSERR) kill(p1_pid);
        if (p2_pid != SYSERR) kill(p2_pid);
        if (pstarv_pid_local != SYSERR) kill(pstarv_pid_local);
        return SHELL_ERROR;
    }

    // Set which process we want to monitor for starvation
    starvingPID = pstarv_pid_local;
    kprintf("Set starvingPID to %d\n", starvingPID);

    // Start the processes
    resume(p1_pid);
    resume(p2_pid);
    resume(pstarv_pid_local);

    // Let them run for a while
    sleep(30);

    // Clean up
    kill(p1_pid);
    kill(p2_pid);
    kill(pstarv_pid_local);
    starvation_prevention = FALSE;  // Reset for Q2

    kprintf("\n======================================================\n");
    kprintf("Context switch-based starvation simulation completed\n");
    kprintf("======================================================\n\n");

    return SHELL_OK;
}

// Q2 solution - time-based starvation prevention
shellcmd starvation_test2(int nargs, char *args[]) {
    pid32 p1_pid_local, p2_pid_local;

    if (nargs > 1) {
        kprintf("Usage: starvation_test2\n");
        return SHELL_ERROR;
    }

    kprintf("\n\n=====================================================\n");
    kprintf("QUESTION 2: TIME-BASED STARVATION PREVENTION\n");
    kprintf("=====================================================\n\n");

    kprintf("Starting time-based starvation simulation at time %d...\n", clktime);

    enable_starvation_fix = FALSE;    // Q2: time-based fix is FALSE
    pstarv_pid = BADPID;
    pstarv_ready_time = 0;
    last_boost_time = 0;

    // INCREASED STACK SIZE FOR SAFETY
    p1_pid_local = create(p1_func, 4096, 40, "P1_Process_Q2", 0);
    p2_pid_local = create(p2_func, 4096, 35, "P2_Process_Q2", 0);
    pstarv_pid = create(pstarv_func_q2, 4096, 25, "Pstarv_Process_Q2", 0);

    if (p1_pid_local == SYSERR || p2_pid_local == SYSERR || pstarv_pid == SYSERR) {
        kprintf("Error: Failed to create one or more processes.\n");
        if (p1_pid_local != SYSERR) kill(p1_pid_local);
        if (p2_pid_local != SYSERR) kill(p2_pid_local);
        if (pstarv_pid != SYSERR) kill(pstarv_pid);
        return SHELL_ERROR;
    }

    // Record the time when Pstarv enters ready queue
    pstarv_ready_time = clktime;
    kprintf("Initializing pstarv_ready_time to %d\n", pstarv_ready_time);

    // Start the processes
    resume(p1_pid_local);
    resume(p2_pid_local);
    resume(pstarv_pid);

    // Let them run for a while
    sleep(30);

    // Clean up
    kill(p1_pid_local);
    kill(p2_pid_local);
    kill(pstarv_pid);

    kprintf("\n======================================================\n");
    kprintf("Time-based starvation simulation completed\n");
    kprintf("======================================================\n\n");

    return SHELL_OK;
}

void resched(void)
{
    struct procent *ptold;
    struct procent *ptnew;

    if (Defer.ndefers > 0) {
        Defer.attempt = TRUE;
        return;
    }

    ptold = &proctab[currpid];

    /* Check for starving processes and update their priorities */
    if (starvation_prevention && starvingPID != 0) {
        pri16 curr_prio = getprio(starvingPID);
        if (curr_prio < MAXPRIO) {
            /* Increase priority by 2 */
            pri16 new_prio = curr_prio + 2;
            updatepriostarv(starvingPID, new_prio);
            
            /* Print the priority boost information */
            kprintf("BOOST: PStarv (PID: %d) priority increased from %d to %d\n", 
                    starvingPID, curr_prio, new_prio);
        }
    }

    if (ptold->prstate == PR_CURR) {
        if (ptold->prprio > firstkey(readylist)) {
            return;
        }

        ptold->prstate = PR_READY;
        insert(currpid, readylist, ptold->prprio);
    }

    currpid = dequeue(readylist);
    ptnew = &proctab[currpid];
    ptnew->prstate = PR_CURR;
    preempt = QUANTUM;

    char *old_name = (ptold->prname[0] != '\0') ? ptold->prname : "prnull";
    char *new_name = (ptnew->prname[0] != '\0') ? ptnew->prname : "prnull";
    
    kprintf("*** CONTEXT SWITCH: From process %d (%s) to %d (%s) ***\n", 
            ptold->prpid, old_name, ptnew->prpid, new_name);

    /* Record when Pstarv enters the ready state */
    if (ptnew->prpid == pstarv_pid && ptnew->prstate == PR_CURR) {
        pstarv_ready_time = clktime;
        kprintf("Initializing pstarv_ready_time to %d\n", pstarv_ready_time);
    }

    ctxsw(&ptold->prstkptr, &ptnew->prstkptr);

    return;
}

unsigned long resched_cntl(int defer)
{
    unsigned long mask;
    mask = Defer.ndefers;
    
    if (defer == DEFER_START) {
        Defer.ndefers++;
    } else if (defer == DEFER_STOP && Defer.ndefers > 0) {
        Defer.ndefers--;
        if (Defer.ndefers == 0 && Defer.attempt) {
            Defer.attempt = FALSE;
            resched();
        }
    }
    return mask;
}