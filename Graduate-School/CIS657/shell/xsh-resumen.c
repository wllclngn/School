#include <xinu.h>

shellcmd xsh_resumen(int nargs, char *args[]) {
    int i;
    pid32 maxprio_pid = -1;
    int maxprio = -1;
    struct procent *prptr;

    if (nargs < 2) {
        kprintf("Usage: resumen <pid1> <pid2> ...\n");
        return 1;
    }

    for (i = 1; i < nargs; i++) {
        pid32 pid = atoi(args[i]);
        prptr = &proctab[pid];
        if (prptr->prprio > maxprio) {
            maxprio = prptr->prprio;
            maxprio_pid = pid;
        }
        resume(pid);
    }

    if (maxprio_pid != -1 && proctab[maxprio_pid].prstate == PR_READY) {
        resched();
    }
    return 0;
}