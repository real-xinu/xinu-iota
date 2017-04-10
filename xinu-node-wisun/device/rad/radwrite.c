/* radwrite.c - radwrite */

#include <xinu.h>

/*------------------------------------------------------------------------
 * radwrite  -  Enqueue a radio packet for transmission
 *------------------------------------------------------------------------
 */
devcall	radwrite (
		struct	dentry *devptr,
		char	*data,
		uint32	count
		)
{
	struct	radcblk *radptr;
	struct	rad_tx_ring_desc *tdescptr;
	intmask	mask;
	static	int32 first = 1;

	mask = disable();

	radptr = &radtab[devptr->dvminor];

	if(first == 1) {
		resume(radptr->ro_process);
		first = 0;
	}

	wait(radptr->osem);

	tdescptr = (struct rad_tx_ring_desc *)radptr->txRing +
							radptr->txTail++;
	if(radptr->txTail >= RAD_TX_RING_SIZE) {
		radptr->txTail = 0;
	}

	tdescptr->data = data;
	tdescptr->datalen = count;

	signal(radptr->qsem);

	restore(mask);

	return count;
}

/*------------------------------------------------------------------------
 * rad_out  -  Process that transmits radio packets
 *------------------------------------------------------------------------
 */
process	rad_out (
		struct	radcblk *radptr
		)
{
	struct	rad_tx_ring_desc *tdescptr;
	struct	netpacket_r *rpkt;
	struct	nbrentry *nbrptr;
	intmask	mask;
	int32	retries;
	int32	free;
	umsg32	msg;
	int32	i;

	while(TRUE) {

		mask = disable();

		wait(radptr->qsem);

		tdescptr = (struct rad_tx_ring_desc *)radptr->txRing +
							radptr->txHead++;
		if(radptr->txHead >= RAD_TX_RING_SIZE) {
			radptr->txHead = 0;
		}

		restore(mask);

		rpkt = (struct netpacket_r *)tdescptr->data;

		free = -1;
		for(i = 0; i < NBRTAB_SIZE; i++) {
			nbrptr = &radptr->nbrtab[i];

			if(nbrptr->state == NBR_STATE_FREE) {
				free = free == -1 ? i : free;
			}

			if(!memcmp(nbrptr->hwaddr, rpkt->net_raddst, 8)) {
				break;
			}
		}

		if(i >= NBRTAB_SIZE) {
			if(free == -1) {
				panic("Neighbor table full!\n");
			}
			nbrptr = &radptr->nbrtab[free];
			memset(nbrptr, NULLCH, sizeof(*nbrptr));
			memcpy(nbrptr->hwaddr, rpkt->net_raddst, 8);
			nbrptr->state = NBR_STATE_USED;
		}

		retries = 0;
		while(retries < 3) {

			write(ETHER0, tdescptr->data, tdescptr->datalen);

			if((rpkt->net_radfc & RAD_FC_FT) == RAD_FC_FT_ACK) {
				break;
			}

			nbrptr->txattempts++;

			msg = recvtime(500);

			if(msg == OK) {
				nbrptr->ackrcvd++;
				nbrptr->etx = (uint16)((((double)nbrptr->txattempts)/nbrptr->ackrcvd)*128);
				break;
			}

			retries++;
		}

		signal(radptr->osem);
	}

	return OK;
}
