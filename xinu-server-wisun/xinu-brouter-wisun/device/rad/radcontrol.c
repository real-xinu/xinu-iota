/* radcontrol.c - radcontrol */

#include <xinu.h>

/*------------------------------------------------------------------------
 * radcontrol  -  Implements control functions for Radio device
 *------------------------------------------------------------------------
 */
devcall	radcontrol (
	struct	dentry *devptr,	/* Entry in device switch table	*/
	int32	func,		/* Control function		*/
	int32	arg1,		/* Argument1 if needed		*/
	int32	arg2		/* Argument2 if needed		*/
	)
{
	struct	radcblk *radptr;	/* Radio table entry pointer	*/

	radptr = &radtab[devptr->dvminor];

	switch(func) {

		/* Get EUI64 address */

		case RAD_CTRL_GET_EUI64:
			memcpy((char *)arg1, radptr->devAddress,
						RAD_ADDR_LEN);
			return OK;

		default:
			return SYSERR;
	}

	return OK;
}
