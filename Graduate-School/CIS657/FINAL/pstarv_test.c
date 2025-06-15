#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Include your header files
#include "Graduate-School/CIS657/FINAL/system/resched.h"
#include "Graduate-School/CIS657/FINAL/system/insert.h"
#include "Graduate-School/CIS657/FINAL/shell/starvation_shell.h"

// Define some XINU-like data structures (simplified)
typedef int pid32;
typedef int pri16;
typedef unsigned long uint32;
typedef int status;

#define OK 1
#define SYSERR -1
#define BADPID -1

#define PR_READY 1
#define PR_CURR 2

#define CLKTICKS_PER_SEC 10 // Simulate clock ticks

// Process table entry (simplified)
struct procent {
    pid32 prpid;
    pri16 prprio;
    int   prstate;
    uint32 prstkptr; // Stack pointer
    uint32 prstklen; // Stack length
};

// Ready list (simplified)
struct qentry {
    pid32 qnext;
    pid32 qprev;
    int32 qkey;
};

// Global variables (simulate XINU globals)
#define NPROC 10 // Max number of processes
struct procent proctab[NPROC];
pid32 currpid = 0; // Current process ID
int enable_starvation_fix = 0; // Global flag
pid32 pstarv_pid = BADPID;
uint32 pstarv_ready_time = 0;

// Mutex to protect shared resources
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Simulated clock
uint32 clktime = 0;

// Function to simulate time passage
void tick(void) {
    pthread_mutex_lock(&mutex);
    clktime++;
    pthread_mutex_unlock(&mutex);
    usleep(100000); // Sleep for 100ms (adjust for desired clock speed)
}

// Queue functions (simplified)
#define NQENT 10
struct qentry queuetab[NQENT];
#define EMPTY -1

pid32 firstid(pid32 q) {
    return queuetab[q].qnext;
}

// Implementations for insert and dequeue (you MUST implement these)
status insert(pid32 pid, qid16 q, int32 key) {
    pthread_mutex_lock(&mutex);
    printf("Simulated insert: PID %d into queue %d with key %d\n", pid, q, key);
    pthread_mutex_unlock(&mutex);
    return OK;
}

pid32 dequeue(pid32 q) {
    pthread_mutex_lock(&mutex);
    printf("Simulated dequeue from queue %d\n", q);
    pthread_mutex_unlock(&mutex);
    return 0;
}

// Simulated process functions (adapt your p1_func, p2_func, pstarv_func)
// The actual implementations are now in starvation_shell.c

// Thread function to run a process
void *process_thread(void *arg) {
    pid32 pid = (pid32)(long)arg; // Cast back to pid32
    switch (pid) {
        case 1: p1_func(); break;
        case 2: p2_func(); break;
        case 3: pstarv_func(); break;
    }
    return NULL;
}

int main() {
    // Initialize process table (simplified)
    for (int i = 0; i < NPROC; i++) {
        proctab[i].prpid = i;
        proctab[i].prstate = 0; // Not running
        proctab[i].prprio = 0;
    }

    // Set up processes (match XINU priorities)
    proctab[1].prprio = 40; // P1
    proctab[2].prprio = 35; // P2
    proctab[3].prprio = 25; // PStarv
    pstarv_pid = 3; // Set global pstarv_pid

    // Create threads for processes
    pthread_t p1_thread, p2_thread, pstarv_thread;
    pthread_create(&p1_thread, NULL, process_thread, (void*)1); // Cast to void*
    pthread_create(&p2_thread, NULL, process_thread, (void*)2);
    pthread_create(&pstarv_thread, NULL, process_thread, (void*)3);

    // Simulate resched every few ticks
    for (int i = 0; i < 100; i++) {
        tick();
        if (i % 5 == 0) {
            resched(); // Call resched to simulate context switch
        }
        usleep(50000); // Sleep for 50ms
    }

    // Clean up threads (detach them)
    pthread_detach(p1_thread);
    pthread_detach(p2_thread);
    pthread_detach(pstarv_thread);

    printf("Simulation complete!\n");
    return 0;
}