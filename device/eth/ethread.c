/* ethread.c - ethread */

#include <xinu.h>

/*------------------------------------------------------------------------
 * ethread  -  Read an incoming packet on Intel Quark Ethernet
 *------------------------------------------------------------------------
 */
devcall	ethread	(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char	*buf,			/* Buffer for the packet	*/
	  int32	len 			/* Size of the buffer		*/
	)
{
	struct	ethcblk *ethptr;	/* Ethertab entry pointer	*/
	struct	eth_q_rx_desc *rdescptr;/* Pointer to the descriptor	*/
	struct	pradpacket *pktptr;	/* Pointer to packet		*/
	uint32	framelen = 0;		/* Length of the incoming frame	*/
	bool8	valid_addr;
	int32	i;

	ethptr = &ethertab[devptr->dvminor];

	while(1) {

		/* Wait until there is a packet in the receive queue */

		wait(ethptr->isem);

		/* Point to the head of the descriptor list */

		rdescptr = (struct eth_q_rx_desc *)ethptr->rxRing +
							ethptr->rxHead;
		pktptr = (struct pradpacket*)rdescptr->buffer1;

		if((memcmp(pktptr->prad_ethdst, ethptr->devAddress, 6) == 0) &&
		   (ntohs(pktptr->prad_ethtype) == 0x0800) &&
		   (pktptr->prad_ipvh == 0x45) &&
		   (pktptr->prad_ipproto == PRAD_IPPROTO) &&
		   (ntohl(pktptr->prad_ipdst) == NetData.ipucast) ) {
		   	valid_addr = TRUE;
		}
		else {
			valid_addr = FALSE;
		}

		/* See if destination address is our unicast address */
		/*
		if(!memcmp(pktptr->net_ethdst, ethptr->devAddress, 6)) {
			valid_addr = TRUE;
		*/
		/* See if destination address is the broadcast address */
		/*
		} else if(!memcmp(pktptr->net_ethdst,
                                    NetData.ethbcast,6)) {
            		valid_addr = TRUE;
		*/
		/* For multicast addresses, see if we should accept */
		/*
    		} else {
			valid_addr = FALSE;
			for(i = 0; i < (ethptr->ed_mcc); i++) {
				if(memcmp(pktptr->net_ethdst,
					ethptr->ed_mca[i], 6) == 0){
					valid_addr = TRUE;
					break;
				}
		        }
		}
		*/
		if(valid_addr == TRUE){ /* Accept this packet */

			int32	i;
			char	*ptr = rdescptr->buffer1;
			/*for(i = 0; i < 100; i++) {
				kprintf("%x ", *ptr);
				ptr++;
			}
			kprintf("\n");
			*/
			/* Get the length of the frame */

			framelen = (rdescptr->status >> 16) & 0x00003FFF;
			framelen = framelen - (14 + 20);

			/* Only return len characters to caller */

			if(framelen > len) {
				framelen = len;
			}

			/* Copy the packet into the caller's buffer */

			memcpy(buf, (char *)rdescptr->buffer1 + 14 + 20, framelen);
		}

        	/* Increment the head of the descriptor list */

		ethptr->rxHead += 1;
		if(ethptr->rxHead >= ETH_QUARK_RX_RING_SIZE) {
			ethptr->rxHead = 0;
		}

		/* Reset the descriptor to max possible frame len */

		rdescptr->buf1size = sizeof(struct netpacket);

		/* If we reach the end of the ring, mark the descriptor	*/

		if(ethptr->rxHead == 0) {
			rdescptr->rdctl1 |= (ETH_QUARK_RDCTL1_RER);
		}

		/* Indicate that the descriptor is ready for DMA input */

		rdescptr->status = ETH_QUARK_RDST_OWN;

		if(valid_addr == TRUE) {
			break;
		}
	}

	/* Return the number of bytes returned from the packet */

	return framelen;

}
