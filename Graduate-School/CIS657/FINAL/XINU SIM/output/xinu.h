/* xinu.h - Generated from XINU headers */
/* Generated on: 2025-06-17 23:12:27 */
/* By user: mol */
#ifndef _XINU_H_
#define _XINU_H_

#include "xinu_stddefs.h"

/* Structure definitions */
struct procent {

        void *prstkptr;
        int prio;
        int prstate;
        char prname[16];
    
};

#endif /* _XINU_H_ */
