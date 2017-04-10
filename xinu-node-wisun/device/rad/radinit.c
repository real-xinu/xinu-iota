/* radinit.c - radinit */

#include <xinu.h>

struct	radcblk	radtab[1];

extern	process	rad_out(struct radcblk *);

/*------------------------------------------------------------------------
 * radinit  -  Initialize the radio device
 *------------------------------------------------------------------------
 */
devcall	radinit(
		struct	dentry *devptr
	       )
{
	struct	radcblk *radptr;	/* Radio control block	*/
	int32	i;

	/* Get pointer to the radio control block */
	radptr = &radtab[devptr->dvminor];

	/* Zero-out the control block */
	memset(radptr, NULLCH, sizeof(struct radcblk));

	/* Set the underlying device */
	radptr->dev = devptr;

	/* Set the device address based on Ethernet MAC */
	memcpy(radptr->devAddress, ethertab[0].devAddress, 3);
	radptr->devAddress[3] = 0xff;
	radptr->devAddress[4] = 0xfe;
	memcpy(&radptr->devAddress[5], &ethertab[0].devAddress[3], 3);

	/* Initialize the tx ring */
	radptr->txRing = getmem(sizeof(struct rad_tx_ring_desc) * RAD_TX_RING_SIZE);
	radptr->txHead = radptr->txTail = 0;
	radptr->osem = semcreate(RAD_TX_RING_SIZE);
	radptr->qsem = semcreate(0);

	for(i = 0; i < NBRTAB_SIZE; i++) {
		memset(&radptr->nbrtab[i], NULLCH, sizeof(struct nbrentry));
		radptr->nbrtab[i].state = NBR_STATE_FREE;
	}

	/* Create a radio output process */
	radptr->ro_process = create(rad_out, 8192, 5000, "rad_out", 1, radptr);

	kprintf("rad_init: done\n");
	return OK;
}
