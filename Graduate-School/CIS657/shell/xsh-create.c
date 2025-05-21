#include <xinu.h>
extern process runforever(void);

shellcmd xsh_create(int nargs, char *args[]) {
    int prio = 20;
    if (nargs == 2) {
        prio = atoi(args[1]);
    }
    resume(create((void *)runforever, 1024, prio, "runforever", 0));
    return 0;
}