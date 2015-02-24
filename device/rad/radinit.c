/* radinit.c - radinit */

#include <xinu.h>

struct	radcblk radtab[1];
struct	rad_fragentry rad_fragtab[RAD_NFRAG];

/*------------------------------------------------------------------------
 * radinit  -  Initialize the Radio device
 *------------------------------------------------------------------------
 */
int32	radinit (
	struct	dentry *devptr		/* Entry in device switch table	*/
	)
{
	struct	radcblk *radptr;	/* Ptr to control block	*/
	int32	i;			/* For loop index	*/

	radptr = &radtab[devptr->dvminor];

	memset(radptr, NULLCH, sizeof(struct radcblk));
	radptr->state = RAD_STATE_DOWN;

	for(i = 0; i < 96; i++) {
		if(!memcmp(ethertab[0].devAddress, xinube_macs[i], 6)) {
			break;
		}
	}
	if(i >= 96) {
		radptr->state = RAD_STATE_DOWN;
		return SYSERR;
	}

	radptr->devAddress[7] = 101+i;

	radptr->rxRingSize = 32;
	radptr->isem = semcreate(32);
	if(radptr->isem == SYSERR) {
		return SYSERR;
	}
	radptr->inPool = mkbufpool(RAD_PKT_SIZE, radptr->rxRingSize);
	if(radptr->inPool == SYSERR) {
		return SYSERR;
	}
	radptr->rxHead = radptr->rxTail = 0;

	radptr->txRingSize = 32;
	radptr->osem = semcreate(32);
	if(radptr->osem == SYSERR) {
		freemem(radptr->rxBufs, radptr->rxRingSize * RAD_PKT_SIZE);
		return SYSERR;
	}
	radptr->outPool = mkbufpool(RAD_PKT_SIZE, radptr->txRingSize);
	if((int32)radptr->outPool == SYSERR) {
		freemem((char *)radptr->rxBufs, radptr->rxRingSize * RAD_PKT_SIZE);
		return SYSERR;
	}

	for(i = 0; i < RAD_NFRAG; i++) {
		rad_fragtab[i].state = RAD_FRAG_FREE;
	}

	radptr->_6lowpan = TRUE;

	radptr->state = RAD_STATE_UP;

	return OK;
}
