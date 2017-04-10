/* radcontrol.c - radcontrol */

#include <xinu.h>

/*------------------------------------------------------------------------
 * radcontrol  -  Control functions for radio device
 *------------------------------------------------------------------------
 */
devcall	radcontrol (
		struct	dentry *devptr,
		int32	func,
		int32	arg1,
		int32	arg2
		)
{
	struct	radcblk *radptr;

	radptr = &radtab[devptr->dvminor];

	switch(func) {

		case RAD_CTRL_GET_HWADDR:
			memcpy((byte *)arg1, radptr->devAddress,
					RAD_ADDR_LEN);
			break;

		default:
			return SYSERR;
	}

	return OK;
}
