/**
 * XINU Simulation Main File
 * Author: wllclngn
 * Date: 2025-06-16 03:12:35 UTC
 */

#include "xinu.h"
#include "pstarv.h"
#include <stdio.h>

/* Constants */
#define NPROC 10
/* Process table and globals */
struct procent proctab[NPROC];
pid32 currpid = 0;
uint32 clktime = 0;
uint32 clkticks = 0;

/* Ready list */
struct {
    pid32 proc_ids[NPROC];
    int count;
} readylist;

/* Function prototypes */
void p1_func(void);
void p2_func(void);
void pstarv_func(void);

/* Update system time */
void update_system_time(void) {
    static DWORD last_tick = 0;
    DWORD current_tick = GetTickCount();
    
    if (last_tick == 0) {
        last_tick = current_tick;
        return;
    }
    
    DWORD elapsed = current_tick - last_tick;
    clkticks += elapsed;
    while (clkticks >= 1000) {
        clktime++;
        clkticks -= 1000;
    }
    
    last_tick = current_tick;
}

/* XINU API implementation for Windows simulation */
void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/* Insert process into ready list */
void insert(pid32 pid, int head, int key) {
    int i;
    
    /* Find insertion point based on priority */
    for (i = 0; i < readylist.count; i++) {
        if (proctab[readylist.proc_ids[i]].prprio < key) {
            break;
        }
    }
    
    /* Shift elements */
    for (int j = readylist.count; j > i; j--) {
        readylist.proc_ids[j] = readylist.proc_ids[j-1];
    }
    
    /* Insert process */
    readylist.proc_ids[i] = pid;
    readylist.count++;
}

/* Remove process from ready list */
pid32 getitem(pid32 pid) {
    int i;
    
    /* Find process in ready list */
    for (i = 0; i < readylist.count; i++) {
        if (readylist.proc_ids[i] == pid) {
            break;
        }
    }
    
    /* Process not found */
    if (i >= readylist.count) {
        return -1;
    }
    
    /* Shift elements */
    for (; i < readylist.count - 1; i++) {
        readylist.proc_ids[i] = readylist.proc_ids[i+1];
    }
    
    readylist.count--;
    return pid;
}

/* Create a new process */
pid32 create(void (*procaddr)(), uint32 stksize, pri16 priority, char *name, uint32 nargs, ...) {
    pid32 pid;
    
    /* Find free slot in process table */
    for (pid = 0; pid < NPROC; pid++) {
        if (proctab[pid].prstate == PR_FREE) {
            break;
        }
    }
    
    if (pid >= NPROC) {
        kprintf("ERROR: No free process slots\n");
        return -1;
    }
    
    /* Initialize process table entry */
    strncpy_s(proctab[pid].prname, sizeof(proctab[pid].prname), name, _TRUNCATE);
    proctab[pid].prpid = pid;
    proctab[pid].prprio = priority;
    proctab[pid].prstate = PR_SUSP;
    
    kprintf("Created process '%s' with PID %d and priority %d\n", name, pid, priority);
    
    return pid;
}

/* Kill a process */
syscall kill(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return -1;
    }
    
    proctab[pid].prstate = PR_FREE;
    return 1;
}

/* Sleep for a specified time */
syscall sleep(uint32 delay) {
    Sleep(delay * 1000);
    return 1;
}

/* Make a process ready */
syscall ready(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return -1;
    }
    
    proctab[pid].prstate = PR_READY;
    insert(pid, 0, proctab[pid].prprio);
    
    return 1;
}

/* Resume a suspended process */
syscall resume(pid32 pid) {
    if (pid < 0 || pid >= NPROC || proctab[pid].prstate != PR_SUSP) {
        return -1;
    }
    
    return ready(pid);
}

/* Get process priority */
pri16 getprio(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return -1;
    }
    
    return proctab[pid].prprio;
}

/* Change process priority */
pri16 chprio(pid32 pid, pri16 newprio) {
    pri16 oldprio;
    
    if (pid < 0 || pid >= NPROC) {
        return -1;
    }
    
    oldprio = proctab[pid].prprio;
    proctab[pid].prprio = newprio;
    
    /* Update position in ready list if process is ready */
    if (proctab[pid].prstate == PR_READY) {
        getitem(pid);
        insert(pid, 0, newprio);
    }
    
    return oldprio;
}

/* Yield processor to another process */
syscall yield(void) {
    /* Select highest priority process from ready list */
    if (readylist.count > 0) {
        pid32 next_pid = readylist.proc_ids[0];
        getitem(next_pid);
        
        /* Put current process back in ready list */
        proctab[currpid].prstate = PR_READY;
        insert(currpid, 0, proctab[currpid].prprio);
        
        /* Switch to new process */
        proctab[next_pid].prstate = PR_CURR;
        pid32 old_pid = currpid;
        currpid = next_pid;
        
        kprintf("*** CONTEXT SWITCH: From process %d (%s) to %d (%s) ***\n", 
            old_pid, proctab[old_pid].prname, currpid, proctab[currpid].prname);
    }
    
    return 1;
}

/* Stub functions for completeness */
syscall receive(void) {
    return 0;
}

syscall recvclr(void) {
    return 1;
}

/* Initialize the system */
void initialize_system(void) {
    int i;
    
    /* Initialize process table */
    for (i = 0; i < NPROC; i++) {
        proctab[i].prstate = PR_FREE;
    }
    
    /* Initialize ready list */
    readylist.count = 0;
    
    /* Initialize time */
    clktime = 0;
    clkticks = 0;
    
    /* Initialize process 0 as current */
    proctab[0].prstate = PR_CURR;
    strncpy_s(proctab[0].prname, sizeof(proctab[0].prname), "prnull", _TRUNCATE);
    proctab[0].prprio = 0;
    currpid = 0;
}

/* Execute a process function */
void execute_process(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return;
    }
    
    /* Set as current process */
    currpid = pid;
    proctab[pid].prstate = PR_CURR;
    
    /* Execute the function based on process name */
    if (strcmp(proctab[pid].prname, "P1") == 0) {
        p1_func();
    }
    else if (strcmp(proctab[pid].prname, "P2") == 0) {
        p2_func();
    }
    else if (strcmp(proctab[pid].prname, "Pstarv") == 0) {
        pstarv_func();
    }
}

/* Main function */
int main(void) {
    /* Initialize the system */
    initialize_system();

    /* Initialize time */
    update_system_time();

    kprintf("\n======================================================\n");
    kprintf("XINU Starvation Prevention Simulation\n");
    kprintf("User: wllclngn\n");
    kprintf("Date: 2025-06-16 03:12:35 UTC\n");
    kprintf("======================================================\n\n");

    /* Create the processes */
    pid32 p1_pid = create(p1_func, 1024, 40, "P1", 0);
    pid32 p2_pid = create(p2_func, 1024, 35, "P2", 0);
    pstarv_pid = create(pstarv_func, 1024, 25, "Pstarv", 0);

    if (p1_pid == SYSERR || p2_pid == SYSERR || pstarv_pid == SYSERR) {
        kprintf("Error creating processes\n");
        return 1;
    }

    /* Set the processes to READY state */
    resume(p1_pid);
    resume(p2_pid);
    resume(pstarv_pid);

    /* Run the simulation */
    int iterations = 0;
    int max_iterations = 100;

    while (iterations < max_iterations) {
        /* Update system time */
        update_system_time();
        
        /* Call resched to switch to the next process */
        resched();

        /* Check if all processes are done */
        int active_count = 0;
        for (int i = 1; i < NPROC; i++) { /* Skip process 0 */
            if (proctab[i].prstate != PR_FREE) {
                active_count++;
            }
        }
        
        if (active_count == 0) {
            break;
        }

        /* Small delay to prevent CPU hogging */
        Sleep(100);
        iterations++;
    }

    kprintf("\n======================================================\n");
    kprintf("Simulation completed after %d iterations\n", iterations);
    kprintf("======================================================\n");

    return 0;
}