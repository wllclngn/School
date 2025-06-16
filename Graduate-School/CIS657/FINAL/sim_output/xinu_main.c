/**
 * XINU Simulation Main File
 * Author: wllclngn
 * Date: 2025-06-15 20:03:50
 */

#include "xinu.h"
#include "pstarv.h"

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
        return SYSERR;
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
        return SYSERR;
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
        return SYSERR;
    }
    
    proctab[pid].prstate = PR_FREE;
    return OK;
}

/* Sleep for a specified time */
syscall sleep(uint32 delay) {
    Sleep(delay * 1000);
    return OK;
}

/* Make a process ready */
syscall ready(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return SYSERR;
    }
    
    proctab[pid].prstate = PR_READY;
    insert(pid, 0, proctab[pid].prprio);
    
    /* If this is the starving process, record its ready time */
    if (pid == pstarv_pid) {
        pstarv_ready_time = clktime;
    }
    
    return OK;
}

/* Resume a suspended process */
syscall resume(pid32 pid) {
    if (pid < 0 || pid >= NPROC || proctab[pid].prstate != PR_SUSP) {
        return SYSERR;
    }
    
    return ready(pid);
}

/* Get process priority */
pri16 getprio(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return SYSERR;
    }
    
    return proctab[pid].prprio;
}

/* Change process priority */
pri16 chprio(pid32 pid, pri16 newprio) {
    pri16 oldprio;
    
    if (pid < 0 || pid >= NPROC) {
        return SYSERR;
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
    
    return OK;
}

/* Stub functions for completeness */
syscall receive(void) {
    return 0;
}

syscall recvclr(void) {
    return OK;
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
    if (strcmp(proctab[pid].prname, "P1") == 0 || 
        strcmp(proctab[pid].prname, "P1_Process") == 0) {
        p1_func();
    }
    else if (strcmp(proctab[pid].prname, "P2") == 0 || 
             strcmp(proctab[pid].prname, "P2_Process") == 0) {
        p2_func();
    }
    else if (strcmp(proctab[pid].prname, "Pstarv") == 0 || 
             strcmp(proctab[pid].prname, "Pstarv_Process") == 0) {
        pstarv_func();
    }
}

/* Main function */
int main(void) {
    /* Initialize the simulation */
    initialize_system();
    
    /* Initialize time */
    update_system_time();
    
    kprintf("\n======================================================\n");
    kprintf("XINU Starvation Prevention Simulation\n");
    kprintf("Using your actual source files from repository\n");
    kprintf("Date: 2025-06-15 20:03:50\n");
    kprintf("======================================================\n\n");
    
    /* Run the starvation test using your implementation */
    char *args[] = {"starvation_test2"};
    starvation_test2(1, args);
    
    /* Main simulation loop */
    int iterations = 0;
    int max_iterations = 100;
    
    while (iterations < max_iterations) {
        /* Update system time */
        update_system_time();
        
        /* Check for time-based starvation prevention */
        check_pstarv_time();
        
        /* If we have ready processes, run the highest priority one */
        if (readylist.count > 0) {
            pid32 next_pid = readylist.proc_ids[0];
            getitem(next_pid);
            
            /* Switch to new process */
            pid32 old_pid = currpid;
            
            if (proctab[old_pid].prstate == PR_CURR) {
                proctab[old_pid].prstate = PR_READY;
                insert(old_pid, 0, proctab[old_pid].prprio);
            }
            
            currpid = next_pid;
            proctab[currpid].prstate = PR_CURR;
            
            kprintf("*** CONTEXT SWITCH: From process %d to %d (%s) ***\n", 
                old_pid, currpid, proctab[currpid].prname);
            
            /* Execute the process */
            execute_process(currpid);
        }
        
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
