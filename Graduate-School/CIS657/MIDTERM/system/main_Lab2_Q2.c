/* main.c - main */
#include <xinu.h>
void waiter();void signaller();
sid32 sem;
pid32 wpid, spid;

void main(void)
{
	sem = semcreate(20);
	wpid=create(waiter, 1024, 40,"waiter", 0);
	spid=create(signaller, 1024, 20,"signaller", 0);
	resume(wpid);
	resume(spid);
	return OK; 
}
void signaller()
{
	while(1) 
	{ 
		kprintf("signaller is running \n ");
		signaln(sem, 5);
		//signal(sem); 
	}
}
void waiter()
{
	int32 i;
	for (i = 1; i <= 2000; i++) 
	{ 
		kprintf("%d - ", i); 
		wait(sem); 
	}
	kill(spid);
}