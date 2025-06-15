#ifndef _PROCESS_H_
#define _PROCESS_H_

/* process.h - isbadpid */

/* Maximum number of processes in the system */
#ifndef NPROC
#define NPROC       8
#endif

/* Process state constants */
#define PR_FREE     0
#define PR_CURR     1
#define PR_READY    2
#define PR_RECV     3
#define PR_SLEEP    4
#define PR_SUSP     5
#define PR_WAIT     6
#define PR_RECTIM   7

/* Miscellaneous process definitions */
#define PNMLEN      16
#define NULLPROC    0
#define BADPID      (-1)
#define MAXKEY      255   // Maximum priority key for queues (add if not present)


/* Process initialization constants */
#define INITSTK     65536
#define INITPRIO    20
#define INITRET     userret

/* Reschedule constants for ready */
#define RESCHED_YES 1
#define RESCHED_NO  0

/* Inline code to check process ID (assumes interrupts are disabled)  */
#define isbadpid(x) ( ((pid32)(x) < 0) || \
              ((pid32)(x) >= NPROC) || \
              (proctab[(x)].prstate == PR_FREE))

/* Number of device descriptors a process can have open */
#define NDESC       5

/* Definition of the process table (multiple of 32 bits) */
struct  procent {
    uint16  prstate;
    pri16   prprio;
    char    *prstkptr;
    char    *prstkbase;
    uint32  prstklen;
    char    prname[PNMLEN];
    uint32  prsem;
    pid32   prparent; // This was identified as present in your file
    umsg32  prmsg;
    bool8   prhasmsg;
    int16   prdesc[NDESC];
};

/* Marker for the top of a process stack (used to help detect overflow)    */
#define STACKMAGIC  0x0A0AAAA9

extern  struct  procent proctab[];
extern  int32   prcount;
extern  pid32   currpid;

// Extern declaration for the ready list ID
extern  qid16   readylist;


#endif /* _PROCESS_H_ */