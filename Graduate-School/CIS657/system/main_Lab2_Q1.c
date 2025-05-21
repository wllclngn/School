#define MAX_ENTRIES  NPROC
#include <xinu.h>
#include <stdio.h>

void runProc(void), readyProc1(void), readyProc2(void), readyProc3(void), readyProc4(void), readyProc5(void);


void main(void)
{

int pid_arr[MAX_ENTRIES];
int ckey_arr[MAX_ENTRIES];
int ppid_arr[MAX_ENTRIES];
int npid_arr[MAX_ENTRIES];
int count = 0; 
int pid, cpid, ckey, ppid, npid, qh_pid, qt_pid;
int i;


resume(create(readyProc1, 1024, 13, "Ready Process 1", 0) );
resume(create(readyProc1, 1024, 15, "Ready Process 2", 0) );
resume(create(readyProc1, 1024, 11, "Ready Process 3", 0) );
resume(create(runProc, 1024, 20, "Running Process", 0) );
resume(create(readyProc1, 1024, 14, "Ready Process 4", 0) );
resume(create(readyProc1, 1024, 12, "Ready Process 5", 0) );


sleepms(100); 

if (queuetab[queuehead(readylist)].qnext != queuetail(readylist)) 
{
    pid = queuetab[queuehead(readylist)].qnext;  
    while (pid != queuetail(readylist) && count < MAX_ENTRIES ) 
    {        
        cpid = pid;
        ckey = queuetab[pid].qkey;
        ppid = queuetab[pid].qprev;
        npid = queuetab[pid].qnext;

    	pid_arr[count] = cpid;
    	ckey_arr[count] = ckey;
    	ppid_arr[count] = ppid;
    	npid_arr[count] = npid;
    	count++;

        pid = npid;  // Move to next process
    }
}

printf("\nQUEUE TABLE");
printf("\n%-5s %-14s %-5s %-5s\n", "PID", "KEY", "PREV", "NXT");
printf("-------------------------------------\n");



// Step 1: Prepare index array
int idx[MAX_ENTRIES];
for (i = 0; i < count; i++) {
    idx[i] = i;
}

// Step 2: Sort indices based on pid_arr values
int j, temp;
for (i = 0; i < count - 1; i++) {
    for (j = i + 1; j < count; j++) {
        if (pid_arr[idx[i]] > pid_arr[idx[j]]) {
            temp = idx[i];
            idx[i] = idx[j];
            idx[j] = temp;
        }
    }
}

// Step 3: Print in sorted order
int k;
for (i = 0; i < count; i++) {
    k = idx[i];
    printf("%-5d %-14d %-5d %-5d\n", pid_arr[k], ckey_arr[k], ppid_arr[k], npid_arr[k]);
}


qh_pid = queuehead(readylist);
qt_pid = queuetail(readylist);


printf("-------------------------------------\n");
printf("CONCEPTUAL BOUNDARY\n");
printf("-------------------------------------\n");
printf("%-5d %-14d %-5d %-5d\n", qh_pid, queuetab[qh_pid].qkey, queuetab[qh_pid].qprev, queuetab[qh_pid].qnext);
printf("%-5d %-14d %-5d %-5d\n", qt_pid, queuetab[qt_pid].qkey, queuetab[qt_pid].qprev, queuetab[qt_pid].qnext);



}
/*------------------------------------------------------------------------*/
void runProc(void)
{
	while( 1 )
	{}
}

void readyProc1(void)
{
	while( 1 )
	{}
}

void readyProc2(void)
{
	while( 1 )
	{}
}

void readyProc3(void)
{
	while( 1 )
	{}
}

void readyProc4(void)
{
	while( 1 )
	{}
}

void readyProc5(void)
{
	while( 1 )
	{}
}


