/* initialize.c - sysinit */

#include <xinu.h> // Master include: pulls in kernel.h, memory.h, process.h, prototypes.h, etc.

/*
 * Note: Global variables like clktime, proctab, currpid, readylist, devtab,
 * memlist, minheap, maxheap are typically defined in other .c files (e.g., a
 * dedicated data.c, or within kernel/proc/device/mem .c files respectively)
 * and declared as 'extern' in their corresponding .h files.
 * This file (initialize.c) should primarily use these externed globals.
 */

/* Global variables for the final exam starvation fix (defined in this file) */
int g_enable_starvation_fix;
pid32 g_pstarv_pid;
uint32 g_pstarv_ready_time;
uint32 g_last_boost_time;


/*------------------------------------------------------------------------
 * nulluser - initialize the system and become the null process
 *------------------------------------------------------------------------
 */
void nulluser(void)
{
    sysinit(); /* Initialize Xinu */

    /* kprintf is declared in kernel.h (and possibly stdio.h via prototypes.h) */
    /* VERSION should be defined in conf.h (generated during compilation) */
    kprintf("\n\nXinu %s\n\n", VERSION);

    /* Null process main loop */
    while (TRUE) {
        asm volatile ("hlt"); /* Halt CPU until an interrupt occurs */
    }
}

/*------------------------------------------------------------------------
 * sysinit - initialize all Xinu data structures and devices
 *------------------------------------------------------------------------
 */
void sysinit(void)
{
    int32 i;
    struct procent *prptr; /* Pointer to process table entry */

    /* Initialize process count (prcount is extern from process.h) */
    prcount = 0;

    /* Initialize process table entries to PR_FREE (proctab is extern from process.h) */
    for (i = 0; i < NPROC; i++) {
        proctab[i].prstate = PR_FREE;
    }

    /* Initialize the Ready list (readylist is extern from kernel.h/queue.h) */
    readylist = newqueue(); /* newqueue() is prototyped in queue.h (included via xinu.h) */

    /* Initialize current process ID (currpid is extern from process.h) */
    currpid = NULLPROC;

    /* Initialize devices (devtab is extern from device.h) */
    for (i = 0; i < NDEVS; i++) {
        if (devtab[i].dvinit != NULL) {
            (devtab[i].dvinit)(&devtab[i]); /* Call device-specific init routine */
        }
    }

    /* Initialize memory manager */
    /* minheap and maxheap (lowercase) are global char* pointers from memory.h */
    /* &end and MAXADDR are from memory.h (via xinu.h) */
    /* memlist is the head of the free list, extern from memory.h */

#ifdef HEAPCHK
    /* This section is for a heap checking version, often a compile-time option */
    memlist.mnext = (struct memblk *)roundmb((uint32)&end);
    memlist.mlength = (uint32)truncmb((uint32)MAXADDR - (uint32)&end);
    if (memlist.mlength < MEMMIN) { /* MEMMIN is from memory.h */
        panic("sysinit: not enough memory for HEAPCHK"); /* panic() from prototypes.h */
    }
    memlist.mnext->mnext = (struct memblk *)NULL;
    memlist.mnext->mlength = memlist.mlength;
#else
    /* Standard heap initialization */
    minheap = (char *)roundmb((uint32)&end);
    maxheap = (char *)truncmb((uint32)MAXADDR);
    if (((uint32)maxheap - (uint32)minheap) < MEMMIN) { /* MEMMIN is from memory.h */
        panic("sysinit: not enough memory"); /* panic() from prototypes.h */
    }
    /* The first call to freemem (usually via getmem) will initialize memlist.mnext and memlist.mlength */
    /* No explicit memlist.mnext/mlength initialization here for standard non-HEAPCHK Xinu memory manager */
    /* However, Xinu typically initializes memlist explicitly: */
    memlist.mnext = (struct memblk *)minheap;
    memlist.mlength = (uint32)maxheap - (uint32)minheap;
    /* If the above causes issues, it implies your freemem/getmem handles the very first setup. */
    /* For robustness with typical Xinu, an explicit init is safer: */
    if (memlist.mlength > 0) { // Only if there's actually memory
        struct memblk *baseblk = (struct memblk *)minheap;
        baseblk->mnext = NULL; // No other blocks initially
        baseblk->mlength = memlist.mlength; // The whole available space
    } else {
        memlist.mnext = NULL; // No memory available
        memlist.mlength = 0;
    }


#endif

    /* Initialize the system clock (clkinit is prototyped, likely in clock.h or prototypes.h) */
    clkinit();

    /* Initialize starvation fix global variables (defined in this file) */
    g_enable_starvation_fix = TRUE; /* Assuming TRUE is 1, defined in kernel.h */
    g_pstarv_pid = BADPID;          /* BADPID from process.h */
    g_pstarv_ready_time = 0;
    g_last_boost_time = 0; /* This will be updated by clkhandler or resched based on clktime */

    /* Initialize the Null process (PID 0) */
    prptr = &proctab[NULLPROC];
    prptr->prstate = PR_CURR;
    prptr->prprio = 0; /* Lowest priority */
    /* strncpy from string.h (via prototypes.h); PNMLEN from process.h */
    strncpy(prptr->prname, "prnull", PNMLEN - 1);
    prptr->prname[PNMLEN - 1] = NULLCH; /* NULLCH from kernel.h (ensure null termination) */
    
    /* NULLSTK from kernel.h */
    prptr->prstkbase = (char *)NULLSTK; 
    prptr->prstklen = NULLSTK;
    /* Stack pointer initialization for x86 (stack grows down): points to the top-most address.
       The context switcher will typically adjust this before running the process.
    */
    prptr->prstkptr = (char *)((uint32)prptr->prstkbase + prptr->prstklen);


    currpid = NULLPROC; /* Set current process to Null process */
    prcount++;          /* Increment active process count */

    kprintf("System initialization complete.\n");
    /* Record current time as of 2025-06-15 02:26:53 UTC */
    kprintf("System initialized by wllclngn at %s %s UTC\n", __DATE__, __TIME__);


    return;
}