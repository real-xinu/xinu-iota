/* panic.c - panic */

#include <xinu.h>

/*------------------------------------------------------------------------
 * panic  -  Display a message and stop all processing
 *------------------------------------------------------------------------
 */
void	panic (
	 char	*msg			/* Message to display		*/
	)
{
	disable();			/* Disable interrupts		*/
	while(1) {

		kprintf("\n\n\rpanic: %s\n\n\r", msg);
		MDELAY(1000);
	}
	while(TRUE) {;}			/* Busy loop forever		*/
}
