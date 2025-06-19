/* shell.c  -  shell */

#include <xinu.h>
#include <stdio.h>
#include "shprototypes.h"		  

/************************************************************************/
/* Xinu shell commands and the function associated with each		*/
/************************************************************************/
const	struct	cmdent	cmdtab[] = {
	{"argecho",		TRUE,	xsh_argecho},
	{"cat",			FALSE,	xsh_cat},
	{"clear",		TRUE,	xsh_clear},
	{"devdump",		FALSE,	xsh_devdump},
	{"echo",		FALSE,	xsh_echo},
	{"exit",		TRUE,	xsh_exit},
	{"help",		FALSE,	xsh_help},
	{"kill",		TRUE,	xsh_kill},
	{"memdump",		FALSE,	xsh_memdump},
	{"memstat",		FALSE,	xsh_memstat},
	{"ps",			FALSE,	xsh_ps},
	{"sleep",		FALSE,	xsh_sleep},
	{"starvation_test_Q1",	FALSE,	starvation_test_Q1},
	{"starvation_test_Q2",	FALSE,	starvation_test_Q2},
	{"?",			FALSE,	xsh_help}
};

uint
