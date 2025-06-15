/* xsh_create.c - xsh_create */

#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern process runforever(void);

/*------------------------------------------------------------
 * xsh_create - Shell command to create a new process. *
 *------------------------------------------------------------
 */

shellcmd xsh_create(int nargs, char *args[]) {
    int priority;
    pid32 pid;

    /* Print help menu and system utilization. */
    if (nargs == 2 && strcmp(args[1], "--help") == 0) {
        printf("Usage: create <priority>\n");
        printf("Creates a new process at the specified priority which loops forever.\n");
        printf("If priority is less than 20, a warning will be displayed.\n");
        printf("If priority is less than 10, the process may make the shell unresponsive.\n");
        return 0;
    }

    if (nargs != 2) {
        fprintf(stderr, "Usage: create <priority>\n");
        return 1;
    }

    /* Validate user input. */
    priority = atoi(args[1]);
    if (priority <= 0) {
        fprintf(stderr, "Invalid priority: %s\n", args[1]);
        return 1;
    }

    /* Prompt user w/ WARNING respective of process' priority. */
    if (priority < 20 && priority >= 10) {
        printf("WARNING: Priority values lower than 20 are typically reserved for shell/system processes.\n");
        printf("         Creating a user process at this priority may interfere with shell responsiveness.\n");
    }
    if (priority < 10) {
        printf("WARNING: Priority values lower than 10 are reserved for critical system processes.\n");
        printf("         Creating a user process at this priority may make the shell or system unresponsive.\n");
    }

    /* Create a process that prints its PID and loops forever */
    pid = create(runforever, 1024, priority, "runforever", 0);
    if (pid == SYSERR) {
        fprintf(stderr, "Failed to create process.\n");
        return 1;
    }
    resume(pid);
    printf("Created process with PID %d at priority %d\n", pid, priority);

    return 0;
}