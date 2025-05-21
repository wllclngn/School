/* xsh_signaln.c - xsh_signaln */

#include <xinu.h>
extern sid32 globalsemaphore;

/*------------------------------------------------------------
 * xsh_signaln - Shell command to signal a semaphore. *
 *------------------------------------------------------------
 */

shellcmd xsh_signaln(int nargs, char *args[]) {
    int n = 1;
    if (nargs == 2) {
        n = atoi(args[1]);
    }
    signaln(globalsemaphore, n);
    kprintf("Signaled semaphore %d times\n", n);
    return 0;
}