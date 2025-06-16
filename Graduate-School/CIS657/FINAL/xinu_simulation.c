#include "xinu.h"
#include <time.h>

/* Function prototypes */
extern void initialize_system(void);
extern shellcmd starvation_test(int nargs, char *args[]);
extern shellcmd starvation_test2(int nargs, char *args[]);
extern void check_pstarv_time(void);

/* Constants */
#define MAX_CMD_LEN 100

/* Get current time as string with YYYY-MM-DD HH:MM:SS UTC format */
void get_current_time(char *time_str, int max_len) {
    time_t rawtime;
    struct tm *timeinfo;
    
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    
    snprintf(time_str, max_len, "%04d-%02d-%02d %02d:%02d:%02d UTC", 
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

/* Update Windows tick count to XINU time */
void update_system_time(void) {
    static DWORD last_tick = 0;
    DWORD current_tick = GetTickCount();
    
    if (last_tick == 0) {
        last_tick = current_tick;
        return;
    }
    
    DWORD elapsed = current_tick - last_tick;
    clkticks += elapsed;
    while (clkticks >= 1000) {
        clktime++;
        clkticks -= 1000;
    }
    
    last_tick = current_tick;
}

/* Main function */
int main(int argc, char *argv[]) {
    char command[MAX_CMD_LEN];
    char current_time[40];
    char *username = "wllclngn";  
    
    /* Process command line arguments for username if provided */
    if (argc > 1) {
        username = argv[1];
    }
    
    /* Initialize the system */
    initialize_system();
    
    /* Initialize time */
    update_system_time();
    
    /* Get current system time for display */
    get_current_time(current_time, sizeof(current_time));
    
    /* Display welcome message */
    kprintf("\n===================================================================\n");
    kprintf("XINU Starvation Prevention Simulation\n");
    kprintf("User: %s\n", username);
    kprintf("Date: %s\n", current_time);
    kprintf("===================================================================\n\n");
    
    kprintf("Available commands:\n");
    kprintf("  starvation_test_Q1 - Run Q1 demonstration (context switch based priority boosting)\n");
    kprintf("  starvation_test_Q2 - Run Q2 demonstration (time based priority boosting)\n");
    kprintf("  exit - Exit the simulation\n\n");
    
    /* Main command loop */
    while (1) {
        /* Display prompt */
        kprintf("xinu> ");
        fflush(stdout);
        
        /* Get command from user */
        if (fgets(command, MAX_CMD_LEN, stdin) == NULL) {
            kprintf("\nEnd of input, exiting...\n");
            break;
        }
        
        /* Remove newline character */
        size_t len = strlen(command);
        if (len > 0 && command[len-1] == '\n') {
            command[len-1] = '\0';
        }
        
        /* Process commands */
        if (strcmp(command, "starvation_test_Q1") == 0) {
            /* Run Q1 demonstration */
            starvation_test_Q1(1, NULL);
            
            /* Simulate running processes */
            int max_iterations = 200;
            int i;
            for (i = 0; i < max_iterations; i++) {
                update_system_time();
                Sleep(100); /* Small delay */
                
                /* Check if all processes are terminated */
                if (pstarv_pid == BADPID || proctab[pstarv_pid].prstate == PR_FREE) {
                    break;
                }
            }
        }
        else if (strcmp(command, "starvation_test_Q2") == 0) {
            /* Run Q2 demonstration */
            starvation_test_Q2(1, NULL);
            
            /* Simulate running processes with time-based boosting */
            int max_iterations = 200;
            int i;
            for (i = 0; i < max_iterations; i++) {
                update_system_time();
                
                /* Call the time-based starvation check function */
                check_pstarv_time();
                
                Sleep(100); /* Small delay */
                
                /* Check if all processes are terminated */
                if (pstarv_pid == BADPID || proctab[pstarv_pid].prstate == PR_FREE) {
                    break;
                }
            }
        }
        else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            kprintf("Exiting simulation...\n");
            break;
        }
        else {
            kprintf("Unknown command: %s\n", command);
            kprintf("Available commands:\n");
            kprintf("  starvation_test_Q1 - Run Q1 demonstration (context switch based)\n");
            kprintf("  starvation_test_Q2 - Run Q2 demonstration (time based)\n");
            kprintf("  exit - Exit the simulation\n");
        }
    }
    
    /* Get current system time for exit message */
    get_current_time(current_time, sizeof(current_time));
    
    kprintf("\n===================================================================\n");
    kprintf("XINU Simulation completed at %s\n", current_time);
    kprintf("===================================================================\n");
    
    return 0;
}