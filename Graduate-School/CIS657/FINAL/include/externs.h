#ifndef _EXTERNS_H_
#define _EXTERNS_H_

#include <xinu.h> // For pid32, uint32 etc.

// Other extern declarations for your system might be here...

// Global variables for the final exam starvation fix
extern int g_enable_starvation_fix;     // TRUE for Q1 (context-switch), FALSE for Q2 (time-based)
extern pid32 g_pstarv_pid;              // PID of the process to monitor for starvation
extern uint32 g_pstarv_ready_time;      // Timestamp when Pstarv entered ready queue (for Q2)
extern uint32 g_last_boost_time;        // Timestamp of the last priority boost (for Q2)

#endif /* _EXTERNS_H_ */