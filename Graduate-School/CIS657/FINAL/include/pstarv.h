#ifndef _PSTARV_H_
#define _PSTARV_H_

#include <xinu.h>

extern pid32 pstarv_pid;
extern bool8 enable_starvation_fix;
extern uint32 pstarv_ready_time;
extern uint32 last_boost_time;

#endif /* _PSTARV_H_ */