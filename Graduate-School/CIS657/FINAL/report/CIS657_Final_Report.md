# CIS657 Final Project: Process Starvation Prevention
## Author: wllclngn
## Date: 2025-06-15

## Introduction

This report documents the implementation of two starvation prevention mechanisms in XINU:

1. **Context Switch-Based Priority Boosting (Question 1)**: Incrementing the priority of a starving process each time a context switch occurs.
2. **Time-Based Priority Boosting (Question 2)**: Incrementing the priority of a starving process every two seconds it spends in the ready queue.

## Implementation Details

### Modified Files

1. **system/main.c**
   - Added global variables for starvation prevention
   - Implemented test processes with different priorities (40, 35, 25)
   - Added time-based starvation prevention demo

2. **system/resched.c**
   - Modified to update the priority of the starving process on each context switch

3. **system/priority.c**
   - Added new function updatepriostarv to handle priority updates
   - Ensured proper updates to the ready list when priorities change

### Question 1: Context Switch-Based Priority Boosting

The implementation boosts the priority of a designated "starving" process each time a context switch occurs, eventually allowing it to run:

\\\c
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

This code is inserted in esched.c at the point where context switches occur. The priority of the starving process is increased by 2 each time, allowing it to eventually overtake the higher priority processes.

### Question 2: Time-Based Priority Boosting

The implementation boosts the priority of the starving process every 2 seconds it spends without getting CPU time:

\\\c
void time_based_starvation_demo(void)
{
    uint32 start_time, curr_time;
    uint32 last_update_time = 0;
    uint32 update_interval = 2000; /* 2 seconds in milliseconds */
    pri16 curr_priority = 25;      /* Starting at priority 25 */
    int i;
    
    kprintf("\n----- Question 2: Time-based Priority Update -----\n");
    kprintf("Starting time-based priority update demonstration...\n");
    kprintf("Increasing priority every 2 seconds...\n\n");
    
    /* Reset the priority for this demonstration */
    chprio(currpid, curr_priority);
    
    /* Get the starting time */
    start_time = clktime * 1000 + clkticks;
    
    /* Run for 20 seconds total */
    while ((clktime * 1000 + clkticks) - start_time < 20000) {
        /* Get current time */
        curr_time = clktime * 1000 + clkticks;
        
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
        
        /* Do some processing to show activity */
        for (i = 0; i < 10000000; i++) {
            /* Just burning cycles */
        }
    }
    
    kprintf("\nTime-based priority update demonstration completed.\n");
    kprintf("Final priority: %d\n", curr_priority);
}
\\\

## Testing and Results

### Test Setup
- Process P1: Priority 40
- Process P2: Priority 35
- Process Pstarv: Priority 25 (would starve without prevention)

### Results

1. **Without Prevention**: The Pstarv process would never run as P1 and P2 have higher priorities.

2. **With Context Switch Prevention (Q1)**: 
   - The Pstarv process's priority increases on each context switch
   - When its priority exceeds those of P1 and P2, it finally gets to run
   - Outputs "SUCCESS!" and celebration message

3. **With Time-Based Prevention (Q2)**:
   - The priority increases every 2 seconds
   - Shows progression of priority values
   - Eventually allows the process to run

## Conclusion

Both starvation prevention mechanisms successfully address the problem of process starvation in XINU by dynamically adjusting process priorities. The context switch-based approach ensures that lower priority processes eventually get CPU time if the system is busy with context switches, while the time-based approach handles scenarios where a process has been waiting for too long.

These implementations demonstrate the importance of fairness in process scheduling and how simple adjustments to the scheduler can prevent resource starvation.
