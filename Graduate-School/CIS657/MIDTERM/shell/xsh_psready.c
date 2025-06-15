/* xsh_psready.c - xsh_psready */

#include <xinu.h>

/*------------------------------------------------------------
 * xsh_psready - Shell command to print the PID Table. *
 *------------------------------------------------------------
 */

shellcmd xsh_psready(int nargs, char *args[]) {
    int32 i;
    struct procent *prptr;

    kprintf("Ready Processes:\n");
    kprintf("%-10s %-10s %-10s\n", "PID", "State", "Priority");
    for (i = 0; i < NPROC; i++) {
        prptr = &proctab[i];
        if (prptr->prstate == PR_READY) {
            kprintf("%-10d %-10s %-10d\n", i, "READY", prptr->prprio);
        }
    }
    return 0;
}