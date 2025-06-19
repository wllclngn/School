/* clkint.S - clkint */

#include <icu.s>

/*------------------------------------------------------------------------
 * clkint - clock interrupt handler
 *------------------------------------------------------------------------
 */
	.text
	.globl	clkint			# Clock interrupt handler
	.extern	clkhandler

clkint:
	pushal			# Push all general-purpose registers
	movb	$EOI,%al	# Clear the interrupt
	outb	%al,$OCW1_2
	call	clkhandler	# Call high-level handler
	popal			# Restore all general-purpose registers
	iret			# Return from interrupt

