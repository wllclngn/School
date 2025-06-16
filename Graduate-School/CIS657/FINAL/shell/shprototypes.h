/* shprototypes.h */

/* Prototypes for shell commands */
shellcmd xsh_argecho(int, char *[]);
shellcmd xsh_cat(int, char *[]);
shellcmd xsh_clear(int, char*[]);
shellcmd xsh_date(int, char*[]);
shellcmd xsh_devdump(int, char*[]);
shellcmd xsh_echo(int, char*[]);
shellcmd xsh_exit(int, char*[]);
shellcmd xsh_help(int, char*[]);
shellcmd xsh_kill(int, char*[]);
shellcmd xsh_memdump(int, char*[]);
shellcmd xsh_memstat(int, char*[]);
shellcmd xsh_ps(int, char*[]);
shellcmd xsh_sleep(int, char*[]);
shellcmd xsh_uptime(int, char*[]);

/* Starvation test commands */
shellcmd starvation_test(int, char*[]);   /* Q1: Context switch-based */
shellcmd starvation_test2(int, char*[]);  /* Q2: Time-based */