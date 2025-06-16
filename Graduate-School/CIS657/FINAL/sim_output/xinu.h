/* xinu.h - Simplified XINU header for Windows simulation */

#ifndef _XINU_H_
#define _XINU_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Windows.h>

/* Constants */
#define NPROC           100     /* Number of user processes */
#define BADPID          -1      /* Invalid process ID */
#define OK              1       /* Normal system call return */
#define SYSERR          -1      /* System call failed return */
#define TRUE            1       /* Boolean true */
#define FALSE           0       /* Boolean false */
#define SHELL_OK        0       /* Shell command return value */
#define SHELL_ERROR     1       /* Shell error return value */
#define CONSOLE         0       /* Console device ID */
#define CLKTICKS_PER_SEC 100    /* Clock ticks per second */

/* Process state constants */
#define PR_FREE         0       /* Process table entry is unused */
#define PR_CURR         1       /* Process is currently running */
#define PR_READY        2       /* Process is on ready queue */
#define PR_RECV         3       /* Process waiting for message */
#define PR_SLEEP        4       /* Process is sleeping */
#define PR_SUSP         5       /* Process is suspended */
#define PR_WAIT         6       /* Process is on semaphore queue */
#define PR_RECTIM       7       /* Process is receiving with timeout */

/* Type definitions */
typedef int pid32;              /* Process ID type */
typedef int pri16;              /* Process priority type */
typedef unsigned int uint32;    /* Unsigned 32-bit integer */
typedef int syscall;            /* System call declaration */
typedef int bool8;              /* Boolean type */
typedef unsigned short uint16;  /* Unsigned 16-bit integer */
typedef int shellcmd;           /* Shell command return type */
typedef int did32;              /* Device ID type */
typedef int intmask;            /* Interrupt mask */

/* Process table entry */
struct procent {
    char    prname[16];         /* Process name */
    pid32   prpid;              /* Process ID */
    pri16   prprio;             /* Process priority */
    int     prstate;            /* Process state */
    void    *prstkptr;          /* Stack pointer */
    pid32   prparent;           /* Process parent */
    uint32  prstklen;           /* Stack length */
    char    *prsem;             /* Semaphore */
    bool8   prhasmsg;           /* Has message */
    uint16  prdesc[16];         /* Device descriptors */
};

/* Function declarations */
extern void kprintf(const char *fmt, ...);
extern syscall ready(pid32 pid);
extern syscall resume(pid32 pid);
extern syscall yield(void);
extern pri16 getprio(pid32 pid);
extern pri16 chprio(pid32 pid, pri16 newprio);
extern syscall kill(pid32 pid);
extern syscall sleep(uint32 delay);
extern pid32 create(void (*procaddr)(), uint32 stksize, pri16 priority, char *name, uint32 nargs, ...);
extern syscall receive(void);
extern syscall recvclr(void);

/* Global variables */
extern struct procent proctab[];
extern pid32 currpid;
extern uint32 clktime;
extern uint32 clkticks;

/* Macro definitions */
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* Special for Windows simulation */
void update_system_time(void);
void insert(pid32 pid, int head, int key);
pid32 getitem(pid32 pid);

#endif /* _XINU_H_ */
