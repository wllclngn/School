/* clkinit.c - clkinit */

#include <xinu.h>
#include <interrupt.h>
#include <clock.h>
#include <pstarv.h>

uint32	clktime;		/* Seconds since boot			*/
uint32	ctr1000 = 0;		/* Milliseconds since boot		*/
qid16	sleepq;			/* Queue of sleeping processes		*/
uint32	preempt;		/* Preemption counter			*/

/*------------------------------------------------------------------------
 * clkinit  -  Initialize the clock and sleep queue at startup
 *------------------------------------------------------------------------
 */
void	clkinit(void)
{
	uint16	intv;			/* Clock rate in KHz		*/

	/* Allocate a queue to hold the delta list of sleeping processes*/

	sleepq = newqueue();

	/* Initialize the preemption count */

	preempt = QUANTUM;

	/* Initialize the time since boot to zero */

	clktime = 0;

	/* Set interrupt vector for the clock to invoke clkdisp */

	set_evec(IRQBASE, (uint32)clkint);

	/* Set the hardware clock: timer 0, 16-bit counter, rate */
	/*   generator mode, counter is binary			*/

	outb(CLKCNTL, 0x34);

	/* Set the clock rate to 1.190 Mhz; this is 1 ms interrupt rate */

	intv = 1193;	/* Using 1193 instead of 1190 to fix clock skew	*/

	/* Must write LSB first, then MSB */

	outb(CLOCK0, (char) (0xff & intv) );
	outb(CLOCK0, (char) (0xff & (intv>>8)));

	return;
}

/*------------------------------------------------------------------------
 * clkhandler - high level clock interrupt handler
 *------------------------------------------------------------------------
 */
void	clkhandler(void)
{
	static uint32	count1000 = 1000;	/* Count to 1000 ms	*/

	/* Decrement the ms counter, and see if a second has passed */

	if((--count1000) <= 0) {

		/* One second has passed, so increment seconds count */

		clktime++;
		
		/* Check for PStarv in Q2 test every second */
		if (pstarv_pid != BADPID && enable_starvation_fix == FALSE) {
		    check_pstarv_time();
		}

		/* Reset the local ms counter for the next second */

		count1000 = 1000;
	}

	/* Decrement the preemption counter, and reschedule when the */
	/*   entire quantum has been used */

	if((--preempt) == 0) {
		preempt = QUANTUM;
		resched();
	}
}
