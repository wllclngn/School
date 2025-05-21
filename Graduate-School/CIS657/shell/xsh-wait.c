#include <xinu.h>
extern sid32 globalsemaphore;
extern process waiter(void);

shellcmd xsh_wait(int nargs, char *args[]) {
    resume(create((void *)waiter, 1024, 20, "waiter", 0));
    return 0;
}