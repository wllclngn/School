#include <xinu.h>

/* Global semaphores */
sid32 globalsemaphore;

/* Process prototypes */
process waiter(void);
process signaller(void);
process runforever(void);
process runafterwait(void);

/* PIDs for lab legacy test */
pid32 wpid, spid;

/* --- Process Definitions --- */

process waiter(void) {
    kprintf("Process PID (wait): %d\n", getpid());
    wait(globalsemaphore);
    while (1) { }
    return OK;
}

process signaller(void) {
    while(1) {
        kprintf("signaller is running\n");
        signaln(globalsemaphore, 5);
        sleep(1); // prevent flooding
    }
    return OK;
}

process runforever(void) {
    kprintf("Process PID: %d\n", getpid());
    while (1) { }
    return OK;
}

process runafterwait(void) {
    sleep(10);
    kprintf("Process PID after sleep: %d\n", getpid());
    while (1) { }
    return OK;
}

/* --- Main Entry Point --- */

void main(void) {
    /* Initialize the global semaphore for shell commands and lab tests */
    globalsemaphore = semcreate(20);

    /* Legacy Lab2_Q2 test: create and resume waiter and signaller processes */
    wpid = create(waiter, 1024, 40, "waiter", 0);
    spid = create(signaller, 1024, 20, "signaller", 0);
    resume(wpid);
    resume(spid);

    /* Normal Xinu main would continue running here... */
}