# CIS657 Final Project: Code Changes Summary
## Author: wllclngn
## Date: 2025-06-15

## Files Modified

### 1. system/main.c
- Added global variables for starvation prevention
- Implemented test processes P1, P2, and Pstarv
- Added functions for demonstrating both starvation prevention methods

### 2. system/resched.c
- Added code to update priority of starving process on context switch (Q1)

### 3. system/priority.c
- Added new function updatepriostarv for starvation prevention
- Enhanced existing priority management functions

## Key Changes Highlighted

### Question 1: Context Switch-Based Priority Boosting
\\\c
/* In resched.c */
/* Check for starving processes and update their priorities */
{
    extern pid32 starvingPID;  /* Defined in main.c */
    extern bool8 starvation_prevention;
    
    if (starvation_prevention && starvingPID != 0) {
        pri16 curr_prio = getprio(starvingPID);
        if (curr_prio < MAXPRIO) {
            /* Increase priority by 2 */
            updatepriostarv(starvingPID, curr_prio + 2);
        }
    }
}
\\\

### Question 2: Time-Based Priority Boosting
\\\c
/* In main.c */
void time_based_starvation_demo(void)
{
    uint32 start_time, curr_time;
    uint32 last_update_time = 0;
    uint32 update_interval = 2000; /* 2 seconds in milliseconds */
    pri16 curr_priority = 25;      /* Starting at priority 25 */
    
    /* Check if it's time to update priority */
    if (curr_time - last_update_time >= update_interval) {
        /* Update priority */
        curr_priority += 1;
        if (curr_priority > MAXPRIO) {
            curr_priority = MAXPRIO;
        }
        
        /* Update the process priority */
        chprio(currpid, curr_priority);
        
        kprintf("Time-based update: Pstarv priority increased to %d after 2 seconds\n", 
                curr_priority);
        
        /* Update the last update time */
        last_update_time = curr_time;
    }
}
\\\

### New Function: updatepriostarv
\\\c
/* In priority.c */
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
\\\

## Building and Running the Solution

1. Compile the modified XINU system:
   \make clean && make\

2. Run the compiled system in an emulator:
   \qemu-system-i386 -nographic -sdl -m 256 -drive file=compile/xinu.boot,media=disk,format=raw\

3. Observe the output showing how the starving process gets to run after priority boosting.
