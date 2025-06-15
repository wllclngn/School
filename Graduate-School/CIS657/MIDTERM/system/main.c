#include <xinu.h>

sid32 globalsemaphore;

process waiter(void);
process signaller(void);
process runforever(void);
process runafterwait(void);

pid32 wpid, spid;

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
        sleep(1);
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

void main(void) {

    /* Init globalsemaphore. */
    globalsemaphore = semcreate(20);
    if (globalsemaphore == SYSERR) {
        kprintf("[main] Failed to create global semaphore!\n");
        return;
    }

    /* Begin XINU shell. */
    kprintf("\n=== Welcome to Xinu CIS657 Midterm System ===\n");
    kprintf("Type 'help' for available commands.\n\n");
    resume(create(shell, 4096, 50, "shell", 1, CONSOLE));

    /* Ensure XINU shell continues operation. */
    while (1) {
        sleep(60);
    }

}