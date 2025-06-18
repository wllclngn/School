/* xinu_stddefs.h - Minimal type definitions for XINU simulation */
/* Generated on: 2025-06-18 00:08:01 */
/* By user: mol */
#ifndef _XINU_STDDEFS_H_
#define _XINU_STDDEFS_H_

/* Basic XINU type definitions */
typedef int pid32;
typedef int sid32;
typedef int qid16;
typedef int did32;
typedef int intmask;
typedef int status;
typedef int message;
typedef int syscall;
typedef int devcall;
typedef int shellcmd;
typedef int process;
typedef int bpid32;
typedef int pri16;
typedef unsigned int memblk;
typedef void (*exchandler)();

/* XINU constants */
#define TRUE        1
#define FALSE       0
#define SYSERR     (-1)
#define OK          1
#define READY       1
#define SUSPENDED   2
#define WAITING     3

#endif /* _XINU_STDDEFS_H_ */
