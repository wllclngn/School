/* pstarv.h - Header for starvation prevention */

#ifndef _PSTARV_H_
#define _PSTARV_H_

#include "xinu.h"

/* Global variables for starvation prevention */
extern bool8 enable_starvation_fix;
extern pid32 pstarv_pid;
extern uint32 pstarv_ready_time;
extern uint32 last_boost_time;

/* Function prototypes */
void check_pstarv_time(void);
shellcmd starvation_test(int nargs, char *args[]);
shellcmd starvation_test2(int nargs, char *args[]);

/* Process function declarations */
void p1_func(void);
void p2_func(void);
void pstarv_func(void);

#endif
