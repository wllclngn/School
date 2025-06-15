/* xsh_createsleep.c - xsh_createsleep */

#include <xinu.h>
extern process runafterwait(void);

/*------------------------------------------------------------
 * xsh_createsleep - Shell command to sleep a process. *
 *------------------------------------------------------------
 */

shellcmd xsh_createsleep(int nargs, char *args[]) {
    int prio = 20;
    if (nargs == 2) {
        prio = atoi(args[1]);
    }
    resume(create((void *)runafterwait, 1024, prio, "runafterwait", 0));
    return 0;
}