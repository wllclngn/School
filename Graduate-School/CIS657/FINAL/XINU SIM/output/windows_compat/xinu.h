#ifdef _WIN32
#include "xinu_windows_compat.h"
#endif

/* xinu.h - include all system header files */

#ifndef _XINU_H_
#define _XINU_H_

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
#include <file.h>
#include <rfilesys.h>
#include <rdisksys.h>
#include <lfilesys.h>
#include <ether.h>
#include <net.h>
#include <ip.h>
#include <arp.h>
#include <udp.h>
#include <dhcp.h>
#include <icmp.h>
#include <tftp.h>
#include <name.h>
#include <shell.h>
#include <date.h>
#include <prototypes.h>
#include <delay.h>
#include <stdio.h>
#include <string.h>

/* Include pstarv.h at the end to avoid circular dependencies */
#include <pstarv.h>

#endif
