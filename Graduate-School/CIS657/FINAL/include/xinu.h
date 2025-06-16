#ifndef _XINU_H_
#define _XINU_H_

#include <stdint.h>
#include <stddef.h> // for size_t, NULL

typedef int32_t int32;
typedef uint32_t uint32;
typedef int16_t qid16;
typedef int32_t pid32;
typedef int32_t sid32;
typedef int32_t bpid32;
typedef uint16_t uint16;
typedef unsigned char bool8;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// Now include all system header files
#include <kernel.h>
#include <conf.h>
#include <process.h>
#include <queue.h>
#include <sched.h>
#include <semaphore.h>
#include <memory.h>
#include <bufpool.h>
#include <clock.h>
#include <mark.h>
#include <ports.h>
#include <uart.h>
#include <tty.h>
#include <device.h>
#include <interrupt.h>
#include <shell.h>
#include <date.h>
#include <prototypes.h>
#include <i386.h>
#include <pci.h>

#endif /* _XINU_H_ */
