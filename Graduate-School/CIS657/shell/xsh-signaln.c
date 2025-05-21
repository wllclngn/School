#include <xinu.h>
extern sid32 globalsemaphore;

shellcmd xsh_signaln(int nargs, char *args[]) {
    int n = 1;
    if (nargs == 2) {
        n = atoi(args[1]);
    }
    signaln(globalsemaphore, n);
    kprintf("Signaled semaphore %d times\n", n);
    return 0;
}