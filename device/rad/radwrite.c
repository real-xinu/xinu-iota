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

void	rad_applyloss(
		byte	ethdst[]
		)
{
	int32	i, j, k;
	byte	bmask;

	bmask = 0x01;
	j = 0;

	for(i = 5; i >= 0; i--) {

		for(k = 0; k < 8; k++) {

			if(i == 0 && k < 2) {
				bmask <<= 1;
				continue;
			}

			if((char)(rand() % 100) < (char)info.link_info[j].probloss) {
				//kprintf("rad_applyloss: DROP %d. prob %d\n", j, info.link_info[j].probloss);
				ethdst[i] &= ~bmask;
			}

			j++;
			bmask <<= 1;
		}
	}
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
	byte	savedethdst[6];
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
			nbrptr->txattempts = 0;
			nbrptr->ackrcvd = 0;
			nbrptr->nextcalc = 1;
			nbrptr->etx = 0;
			nbrptr->state = NBR_STATE_USED;
		}

		memcpy(savedethdst, rpkt->net_ethdst, 6);

		retries = 0;
		while(retries < 3) {

			memcpy(rpkt->net_ethdst, savedethdst, 6);
			rad_applyloss(rpkt->net_ethdst);

			write(ETHER0, tdescptr->data, tdescptr->datalen);

			if((rpkt->net_radfc & RAD_FC_FT) == RAD_FC_FT_ACK) {
				break;
			}

			if(!(rpkt->net_radfc & RAD_FC_AR)) {
				break;
			}

			nbrptr->txattempts++;
			nbrptr->nextcalc--;

			radptr->rowaiting = TRUE;
			radptr->rowaitseq = rpkt->net_radseq;
			memcpy(radptr->rowaitaddr, rpkt->net_raddst, 8);
			msg = recvtime(50);
			radptr->rowaiting = FALSE;

			if(msg == OK) {
				nbrptr->ackrcvd++;

				if((nbrptr->nextcalc <= 0) && ((nbrptr->etx == 0) || (clktime >= nbrptr->calctime))) {

					if(nbrptr->ackrcvd == 0) {
						if(nbrptr->etx == 0) {
							//nbrptr->etx = 511.99;
							nbrptr->etx = 511;
						}
						else {
							//nbrptr->etx = ((double)511.99 + 7.0*nbrptr->etx)/8;
							nbrptr->etx = (511 + (7*nbrptr->etx))/8;
						}
					}
					else {
						if(nbrptr->etx == 0) {
							nbrptr->etx = nbrptr->txattempts/nbrptr->ackrcvd;
						}
						else {
							//nbrptr->etx = (((double)nbrptr->txattempts/nbrptr->ackrcvd) +
							//			7.0*nbrptr->etx)/8;
							nbrptr->etx = ((nbrptr->txattempts/nbrptr->ackrcvd) +
									(7*nbrptr->etx))/8;
						}
					}
					nbrptr->nextcalc = 4;
					nbrptr->calctime = clktime + 60;
					/*
					kprintf("Neighbor: ");
					for(i = 0; i < 8; i++) {
						kprintf("%02x ", nbrptr->hwaddr[i]);
					}
					kprintf(", ETX = %d, %d, %d\n", (uint16)(128.0*nbrptr->etx), nbrptr->txattempts, nbrptr->ackrcvd);
					*/
				}
				break;
			}

			retries++;
		}

		signal(radptr->osem);
	}

	return OK;
}
