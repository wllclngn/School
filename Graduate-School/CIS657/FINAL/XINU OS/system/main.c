/*  main.c  - main */

#include <xinu.h>
#include <stdio.h>

/* Global variables for starvation prevention */
pid32 starvingPID = 0;      /* ID of the process that should not be starved */
bool8 starvation_prevention = TRUE; /* Flag to control starvation prevention */

/* Function prototypes */
void process1(void);
void process2(void);
void starving_process(void);
void time_based_starvation_demo(void);

/*------------------------------------------------------------------------
 * main - Main function for CIS657 Final Project: Starvation Prevention
 *------------------------------------------------------------------------
 */
int main(int argc, char **argv)
{
	pid32 p1, p2, pstarv;
	
	kprintf("\n\n");
	kprintf("=======================================================\n");
	kprintf("CIS657 Final Project: Starvation Prevention Demonstration\n");
	kprintf("=======================================================\n\n");
	
	kprintf("Creating processes for demonstrating starvation prevention...\n");
	
	/* Create two high-priority processes */
	p1 = create(process1, 4096, 40, "P1", 0);
	p2 = create(process2, 4096, 35, "P2", 0);
	
	/* Create a low-priority process that would starve without prevention */
	pstarv = create(starving_process, 4096, 25, "Pstarv", 0);
	
	/* Set the starving process ID for the prevention mechanism */
	starvingPID = pstarv;
	
	/* Start the processes */
	kprintf("Starting processes...\n\n");
	resume(p1);
	resume(p2);
	resume(pstarv);
	
	/* Create a system shell */
	resume(create(shell, 8192, 50, "shell", 1, CONSOLE));
	
	/* Wait for shell to exit and recreate it */
	recvclr();
	while (TRUE) {
		uint32 retval = receive();
		kprintf("\n\n\rMain process recreating shell\n\n\r");
		resume(create(shell, 4096, 1, "shell", 1, CONSOLE));
	}
	
	return OK;
}

/*------------------------------------------------------------------------
 * process1 - High priority process for demonstrating context switching
 *------------------------------------------------------------------------
 */
void process1(void)
{
	int i;
	
	for (i = 0; i < 20; i++) {
		kprintf("Process P1 (PID: %d, Priority: %d) running...\n", 
				currpid, getprio(currpid));
		
		/* Simulate some work */
		sleepms(300);
		
		/* Explicitly yield control to cause context switch */
		resched();
	}
	
	kprintf("Process P1 completed\n");
}

/*------------------------------------------------------------------------
 * process2 - Second high priority process for demonstrating context switching
 *------------------------------------------------------------------------
 */
void process2(void)
{
	int i;
	
	for (i = 0; i < 20; i++) {
		kprintf("Process P2 (PID: %d, Priority: %d) running...\n", 
				currpid, getprio(currpid));
		
		/* Simulate some work */
		sleepms(300);
		
		/* Explicitly yield control to cause context switch */
		resched();
	}
	
	kprintf("Process P2 completed\n");
}

/*------------------------------------------------------------------------
 * starving_process - Low priority process that would starve without prevention
 *------------------------------------------------------------------------
 */
void starving_process(void)
{
	kprintf("\n!!! SUCCESS! Starving process (PID: %d) is finally running !!!\n", 
			currpid);
	kprintf("!!! Celebration time! You'll get a good grade! !!!\n\n");
	
	/* After running once, demonstrate time-based starvation prevention (Q2) */
	time_based_starvation_demo();
	
	kprintf("Starving process completed\n");
}

/*------------------------------------------------------------------------
 * time_based_starvation_demo - Demo for Question 2 (time-based priority increment)
 *------------------------------------------------------------------------
 */
void time_based_starvation_demo(void)
{
	uint32 start_time, curr_time;
	uint32 last_update_time = 0;
	uint32 update_interval = 2000; /* 2 seconds in milliseconds */
	pri16 curr_priority = 25;      /* Starting at priority 25 */
	int i;
	
	kprintf("\n----- Question 2: Time-based Priority Update -----\n");
	kprintf("Starting time-based priority update demonstration...\n");
	kprintf("Increasing priority every 2 seconds...\n\n");
	
	/* Reset the priority for this demonstration */
	chprio(currpid, curr_priority);
	
	/* Get the starting time */
	start_time = clktime * 1000 + clkticks;
	
	/* Run for 20 seconds total */
	while ((clktime * 1000 + clkticks) - start_time < 20000) {
		/* Get current time */
		curr_time = clktime * 1000 + clkticks;
		
		/* Check if it's time to update priority */
		if (curr_time - last_update_time >= update_interval) {
			/* Update priority */
			curr_priority += 1;
			if (curr_priority > MAXPRIO) {
				curr_priority = MAXPRIO;
			}
			
			/* Update the process priority */
			chprio(currpid, curr_priority);
			
			kprintf("Time-based update: Pstarv priority increased to %d after 2 seconds\n", 
					curr_priority);
			
			/* Update the last update time */
			last_update_time = curr_time;
		}
		
		/* Do some processing to show activity */
		for (i = 0; i < 10000000; i++) {
			/* Just burning cycles */
		}
	}
	
	kprintf("\nTime-based priority update demonstration completed.\n");
	kprintf("Final priority: %d\n", curr_priority);
}
