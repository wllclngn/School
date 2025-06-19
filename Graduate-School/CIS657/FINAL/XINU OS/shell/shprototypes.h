/* shprototypes.h - Shell command prototypes */

/* Shell command function prototypes */
shellcmd xsh_argecho(int, char *[]);
shellcmd xsh_arp(int, char *[]);
shellcmd xsh_cat(int, char *[]);
shellcmd xsh_clear(int, char *[]);
shellcmd xsh_date(int, char *[]);
shellcmd xsh_devdump(int, char *[]);
shellcmd xsh_echo(int, char *[]);
shellcmd xsh_exit(int, char *[]);
shellcmd xsh_help(int, char *[]);
shellcmd xsh_kill(int, char *[]);
shellcmd xsh_memdump(int, char *[]);
shellcmd xsh_memstat(int, char *[]);
shellcmd xsh_ps(int, char *[]);
shellcmd xsh_sleep(int, char *[]);
shellcmd xsh_uptime(int, char*[]);
shellcmd starvation_test_Q1(int, char *[]);    /* Question 1 test command */
shellcmd starvation_test_Q2(int, char *[]);    /* Question 2 test command */
shellcmd starvation_test_Q1_entry(int, char *[]);  /* Q1 specific entry */

/* Process function prototypes */
void p1_func(void);
void p2_func(void);
void pstarv_func_q1(void);
void pstarv_func_q2(void);
void p1_func_q1(void);
void p2_func_q1(void);
void pstarv_func_q1_entry(void);
