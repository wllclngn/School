#ifndef _XINU_SIMULATION_H_
#define _XINU_SIMULATION_H_

// Standard C headers - MSVC should provide these from system paths
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

// Windows API
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Include project's own non-standard headers (e.g., kernel.h, process.h)
// These are included AFTER system headers.
#include "include/bufpool.h" // Project header: bufpool.h
#include "include/clock.h" // Project header: clock.h
#include "include/conf.h" // Project header: conf.h
/* SKIPPING project's custom standard header: ctype.h (using MSVC system version) */
#include "include/date.h" // Project header: date.h
#include "include/device.h" // Project header: device.h
#include "include/i386.h" // Project header: i386.h
#include "include/icu.h" // Project header: icu.h
#include "include/interrupt.h" // Project header: interrupt.h
#include "include/kernel.h" // Project header: kernel.h
#include "include/mark.h" // Project header: mark.h
#include "include/memory.h" // Project header: memory.h
#include "include/pci.h" // Project header: pci.h
#include "include/ports.h" // Project header: ports.h
#include "include/process.h" // Project header: process.h
#include "include/prototypes.h" // Project header: prototypes.h
#include "include/pstarv.h" // Project header: pstarv.h
#include "include/queue.h" // Project header: queue.h
#include "include/sched.h" // Project header: sched.h
#include "include/semaphore.h" // Project header: semaphore.h
#include "include/shell.h" // Project header: shell.h
/* SKIPPING project's custom standard header: stdarg.h (using MSVC system version) */
#include "include/stddef.h" // Project header: stddef.h
/* SKIPPING project's custom standard header: stdio.h (using MSVC system version) */
/* SKIPPING project's custom standard header: stdlib.h (using MSVC system version) */
/* SKIPPING project's custom standard header: string.h (using MSVC system version) */
#include "include/tty.h" // Project header: tty.h
#include "include/uart.h" // Project header: uart.h
#include "include/xinu.h" // Project header: xinu.h
// Basic fallbacks if not defined by project headers
#ifndef PNMLEN
    #define PNMLEN 16
#endif
#ifndef NPROC
    #define NPROC 64
#endif
// Add other essential Xinu constants/types definitions as fallbacks if they are commonly needed
// and not reliably defined in your project's core headers (like kernel.h) for the simulation.
// For example:
// #ifndef SYSERR
//     #define SYSERR (-1)
// #endif
// #ifndef OK
//     #define OK (1)
// #endif

// Forward declarations for xinu_init.c functions or core Xinu API used by simulation
void initialize_system(void);
void kprintf(const char *format, ...);
// Add other prototypes for functions defined in your Xinu C code that xinu_simulation.c might call.

#endif // _XINU_SIMULATION_H_
