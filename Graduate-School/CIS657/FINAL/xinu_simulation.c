#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "xinu_includes.h"

/* Global variables for simulation */
pid32 pstarv_pid = BADPID;          /* PID of process that might starve */
bool8 enable_starvation_fix = TRUE; /* Enable starvation prevention */
uint32 pstarv_ready_time = 0;       /* Time when PStarv became ready */
uint32 last_boost_time = 0;         /* Last time priority was boosted */
char current_username[100];         /* Current Windows username */
bool8 shutdown_requested = FALSE;   /* Flag to request shutdown */

/* Forward declarations */
int starvation_test_Q1(int nargs, char *args[]);
int starvation_test_Q2(int nargs, char *args[]);
void process_command(char *buffer);
pid32 create(void (*funcaddr)(), uint32 ssize, pri16 priority, char *name, uint32 nargs, ...);
void initialize_system(void);

/**
 * process_command - Parse and execute user commands
 */
void process_command(char *buffer) {
    /* Remove trailing newline */
    buffer[strcspn(buffer, "\n")] = '\0';
    
    if (strcmp(buffer, "starvation_test_Q1") == 0) {
        starvation_test_Q1(0, NULL);
    } else if (strcmp(buffer, "starvation_test_Q2") == 0) {
        starvation_test_Q2(0, NULL);
    } else if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) {
        shutdown_requested = TRUE;
        printf("Shutting down simulation...\n");
    } else if (strlen(buffer) > 0) {
        printf("Unknown command: %s\n", buffer);
        printf("Available commands:\n");
        printf("  starvation_test_Q1 - Run Q1 demonstration (context switch based priority boosting)\n");
        printf("  starvation_test_Q2 - Run Q2 demonstration (time based priority boosting)\n");
        printf("  exit - Exit the simulation\n");
    }
}

/**
 * initialize_system - Set up the simulation environment
 */
void initialize_system(void) {
    /* Initialize all kernel data structures */
    currpid = 0;
    
    /* Initialize process table */
    for (int i = 0; i < NPROC; i++) {
        proctab[i].prstate = PR_FREE;
        strcpy(proctab[i].prname, "");
        proctab[i].prprio = 0;
    }
    
    /* Set up the null process */
    proctab[0].prstate = PR_CURR;
    strcpy(proctab[0].prname, "null");
    proctab[0].prprio = 0;
    
    /* Initialize ready list */
    readylist = newqueue();
    
    /* Initialize starvation prevention variables */
    pstarv_pid = BADPID;
    enable_starvation_fix = TRUE;
    pstarv_ready_time = 0;
    last_boost_time = 0;
}

/**
 * Create a process in the simulation
 */
pid32 create(void (*funcaddr)(), uint32 ssize, pri16 priority, char *name, uint32 nargs, ...) {
    /* Find a free slot in process table */
    pid32 pid = BADPID;
    for (int i = 1; i < NPROC; i++) {
        if (proctab[i].prstate == PR_FREE) {
            pid = i;
            break;
        }
    }
    
    if (pid == BADPID) {
        printf("Error: No free process slots\n");
        return BADPID;
    }
    
    /* Initialize process table entry */
    proctab[pid].prstate = PR_SUSP;    /* Initial state is suspended */
    proctab[pid].prprio = priority;
    strcpy(proctab[pid].prname, name);
    
    printf("Created process %s with PID %d and priority %d\n", name, pid, priority);
    
    return pid;
}

/**
 * main - Entry point for simulation
 */
int main(int argc, char *argv[]) {
    char input_buffer[256];
    time_t now;
    struct tm timeinfo;
    char timestr[64];
    
    /* Store username if provided */
    if (argc >= 2) {
        strncpy(current_username, argv[1], sizeof(current_username)-1);
        current_username[sizeof(current_username)-1] = '\0';
    } else {
        strcpy(current_username, "user");
    }
    
    /* Get current time */
    time(&now);
    gmtime_s(&timeinfo, &now);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    /* Print header */
    printf("\n===================================================================\n");
    printf("XINU Starvation Prevention Simulation\n");
    printf("User: %s\n", current_username);
    printf("Date: %s UTC\n", timestr);
    printf("===================================================================\n\n");
    
    /* Initialize system */
    initialize_system();
    
    /* Display available commands */
    printf("Available commands:\n");
    printf("  starvation_test_Q1 - Run Q1 demonstration (context switch based priority boosting)\n");
    printf("  starvation_test_Q2 - Run Q2 demonstration (time based priority boosting)\n");
    printf("  exit - Exit the simulation\n\n");
    
    /* Process commands */
    while (!shutdown_requested) {
        printf("xinu> ");
        fflush(stdout);  /* Important: Flush the buffer to ensure prompt is displayed */
        
        /* Use fgets for line-based input with NULL check */
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            printf("\nEnd of input, exiting...\n");
            break;
        }
        
        process_command(input_buffer);
    }
    
    return 0;
}