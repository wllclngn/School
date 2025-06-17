#ifndef _SEMAPHORE_H_ // Or any other unique guard name
#define _SEMAPHORE_H_

/* semaphore.h - isbadsem */

#ifndef NSEM
#define NSEM        50  /* number of semaphores, if not defined */
#endif

#include <xinu.h>

#define S_FREE      0   /* semaphore table entry is available   */
#define S_USED      1   /* semaphore table entry is in use  */

/* Semaphore table entry */

struct  sentry  {
    byte    sstate;     /* S_FREE or S_USED         */
    int32   scount;     /* count for this semaphore     */
    qid16   squeue;     /* qid of processes that are waiting    */
                        /*    on this semaphore       */
};
extern  struct  sentry  semtab[];
extern  sid32   sem_next;

#define isbadsem(s) ((int32)(s) < 0 || (s) >= NSEM)

#endif /* _SEMAPHORE_H_ */
