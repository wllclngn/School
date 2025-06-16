/**
 * XINU Starvation Problem Simulation
 * CIS657 Final Project
 * Author: wllclngn
 * Date: 2025-06-15
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

/* Constants */
#define MAXPRIO     100     /* Maximum process priority */
#define MINPRIO     0       /* Minimum process priority */
#define MAX_PROCS   10      /* Maximum number of processes */
#define SYSERR     -1       /* Error code */
#define OK          1       /* Success code */

/* Process states */
#define PR_FREE     0       /* Process table entry is unused */
#define PR_CURR     1       /* Process is currently running  */
#define PR_READY    2       /* Process is on ready queue     */
#define PR_SUSP     3       /* Process is suspended          */
#define PR_WAIT     4       /* Process is waiting            */
#define PR_SLEEP    5       /* Process is sleeping           */

/* Types */
typedef int pid32;          /* Process ID type */
typedef int pri16;          /* Priority type */
typedef int bool;           /* Boolean type */
typedef int syscall;        /* System call return type */

#define TRUE        1
#define FALSE       0

/* Global variables */
time_t start_time;
DWORD clktime;              /* Current "system" time in seconds */
DWORD clkticks;             /* Milliseconds beyond seconds */

/* Process information */
typedef struct {
    char    name[16];       /* Process name */
    pid32   pid;            /* Process ID */
    pri16   priority;       /* Priority */
    int     state;          /* State */
    int     runtime;        /* Simulated runtime so far (ms) */
    int     total_runtime;  /* Total simulated runtime (ms) */
    char    *message;       /* Message when process runs */
    int     executed;       /* Whether the process has ever executed */
    int     celebration;    /* Whether the process showed celebration message */
    time_t  wait_start;     /* When process started waiting */
} Process;

/* Global variables for simulation */
Process processes[MAX_PROCS];
int process_count = 0;
pid32 current_pid = -1;
pid32 starvingPID = -1;
bool starvation_prevention = TRUE;

/* Function prototypes */
void init_simulation(void);
void run_simulation(void);
void print_process_status(void);
void update_priority_on_context_switch(void);
void update_priority_based_on_time(void);
pid32 create_process(const char *name, pri16 priority, int runtime, const char *message);
pid32 select_next_process(void);
void context_switch(pid32 old, pid32 new);

/**
 * Main function - entry point for simulation
 */
int main(void) {
    printf("=======================================================\n");
    printf("CIS657 Final Project: Starvation Prevention Demonstration\n");
    printf("=======================================================\n\n");
    
    init_simulation();
    run_simulation();
    
    printf("\nSimulation completed successfully.\n");
    return 0;
}

/**
 * Initialize the simulation
 */
void init_simulation(void) {
    int i;
    
    /* Initialize system time */
    start_time = time(NULL);
    clktime = 0;
    clkticks = 0;
    
    /* Initialize process array */
    for (i = 0; i < MAX_PROCS; i++) {
        processes[i].state = PR_FREE;
    }
    
    printf("Creating processes for demonstrating starvation prevention...\n");
    
    /* Create processes with different priorities */
    pid32 p1 = create_process("P1", 40, 5000, "Process P1 (high priority) running...");
    pid32 p2 = create_process("P2", 35, 5000, "Process P2 (medium priority) running...");
    starvingPID = create_process("Pstarv", 25, 2500, "Process Pstarv (low priority) running...");
    
    printf("Starting processes...\n\n");
    
    /* Set all processes to ready */
    for (i = 0; i < process_count; i++) {
        processes[i].state = PR_READY;
        processes[i].wait_start = time(NULL);
    }
}

/**
 * Run the simulation
 */
void run_simulation(void) {
    int active_processes = process_count;
    time_t sim_start = time(NULL);
    time_t last_time_check = time(NULL);
    
    while (active_processes > 0) {
        /* Update system time */
        clktime = (DWORD)(time(NULL) - start_time);
        
        /* Check for time-based priority updates (for Q2) every second */
        if (time(NULL) > last_time_check) {
            update_priority_based_on_time();
            last_time_check = time(NULL);
        }
        
        /* Select next process to run */
        pid32 old_pid = current_pid;
        current_pid = select_next_process();
        
        if (current_pid != -1) {
            /* Context switch if needed */
            if (old_pid != current_pid) {
                context_switch(old_pid, current_pid);
            }
            
            /* Simulate process running */
            Process *proc = &processes[current_pid];
            
            /* If first time running, show special message */
            if (!proc->executed) {
                printf("\n*** Process %s (PID %d, Priority %d) gets CPU for the first time ***\n", 
                       proc->name, proc->pid, proc->priority);
                proc->executed = TRUE;
                
                /* Special message for Pstarv */
                if (proc->pid == starvingPID && !proc->celebration) {
                    printf("\n!!! SUCCESS! Starving process (PID: %d) is finally running !!!\n", proc->pid);
                    printf("!!! Celebration time! You'll get a good grade! !!!\n\n");
                    proc->celebration = TRUE;
                }
            }
            
            /* Show the process message */
            printf("%s (Priority: %d)\n", proc->message, proc->priority);
            
            /* Simulate work by sleeping */
            Sleep(200);  /* Sleep to simulate work but not too long */
            
            /* Update runtime for the process */
            proc->runtime += 200;
            
            /* Check if the process has completed its total runtime */
            if (proc->runtime >= proc->total_runtime) {
                printf("Process %s completed its execution\n", proc->name);
                proc->state = PR_FREE;
                active_processes--;
            }
        }
        
        /* Exit if simulation runs too long (safety) */
        if (time(NULL) - sim_start > 60) {
            printf("Simulation timeout after 60 seconds\n");
            break;
        }
    }
}

/**
 * Create a new process for the simulation
 */
pid32 create_process(const char *name, pri16 priority, int runtime, const char *message) {
    if (process_count >= MAX_PROCS) {
        printf("Error: Process table full\n");
        return SYSERR;
    }
    
    Process *proc = &processes[process_count];
    strncpy(proc->name, name, 15);
    proc->name[15] = '\0';
    proc->pid = process_count;
    proc->priority = priority;
    proc->state = PR_SUSP;
    proc->runtime = 0;
    proc->total_runtime = runtime;
    proc->message = _strdup(message);
    proc->executed = FALSE;
    proc->celebration = FALSE;
    
    printf("Created process '%s' with PID %d and priority %d\n", name, proc->pid, priority);
    
    return process_count++;
}

/**
 * Select the highest priority ready process
 */
pid32 select_next_process(void) {
    int i;
    pid32 highest_pid = -1;
    pri16 highest_priority = -1;
    
    /* Find the highest priority ready process */
    for (i = 0; i < process_count; i++) {
        if (processes[i].state == PR_READY && processes[i].priority > highest_priority) {
            highest_priority = processes[i].priority;
            highest_pid = i;
        }
    }
    
    return highest_pid;
}

/**
 * Perform context switch between processes
 */
void context_switch(pid32 old_pid, pid32 new_pid) {
    if (old_pid != -1) {
        /* Put old process back to ready state */
        processes[old_pid].state = PR_READY;
        processes[old_pid].wait_start = time(NULL);
    }
    
    /* Update new process state */
    processes[new_pid].state = PR_CURR;
    
    /* Update priorities on context switch (for Q1) */
    update_priority_on_context_switch();
    
    /* Print process status */
    print_process_status();
}

/**
 * Print current process status
 */
void print_process_status(void) {
    int i;
    printf("\n----- Process Status -----\n");
    for (i = 0; i < process_count; i++) {
        printf("PID: %d, Name: %s, Priority: %d, State: %d\n", 
               processes[i].pid, processes[i].name, processes[i].priority, processes[i].state);
    }
    printf("-------------------------\n");
}

/**
 * Update priority of starving process on context switch (Q1)
 */
void update_priority_on_context_switch(void) {
    if (starvation_prevention && starvingPID != -1 && processes[starvingPID].state == PR_READY) {
        processes[starvingPID].priority += 2;
        if (processes[starvingPID].priority > MAXPRIO) {
            processes[starvingPID].priority = MAXPRIO;
        }
        printf("Q1: Process %s priority increased to %d on context switch\n", 
               processes[starvingPID].name, processes[starvingPID].priority);
    }
}

/**
 * Update priority of starving process based on wait time (Q2)
 */
void update_priority_based_on_time(void) {
    if (starvingPID != -1 && processes[starvingPID].state == PR_READY) {
        /* Get the time the process has been waiting */
        time_t current = time(NULL);
        time_t wait_time = current - processes[starvingPID].wait_start;
        
        /* Increase priority every 2 seconds of wait time */
        if (wait_time >= 2) {
            processes[starvingPID].priority += 1;
            if (processes[starvingPID].priority > MAXPRIO) {
                processes[starvingPID].priority = MAXPRIO;
            }
            
            printf("Q2: Process %s priority increased to %d after 2 seconds wait\n", 
                   processes[starvingPID].name, processes[starvingPID].priority);
                   
            /* Reset the wait time counter */
            processes[starvingPID].wait_start = current;
        }
    }
}
