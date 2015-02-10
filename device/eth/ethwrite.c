/* ethwrite.c - ethwrite */

#include <xinu.h>
extern	int32 beserver;

/*------------------------------------------------------------------------
 * ethwrite  -  enqueue packet for transmission on Intel Quark Ethernet
 *------------------------------------------------------------------------
 */
devcall	ethwrite	(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char	*buf,			/* Buffer that hols a packet	*/
	  int32	len 			/* Length of the packet		*/
	)
{
	struct	ethcblk *ethptr;	/* Pointer to control block	*/
	struct	eth_q_csreg *csrptr;	/* Address of device CSRs	*/
	volatile struct	eth_q_tx_desc *descptr; /* Ptr to descriptor	*/
	struct	etherPkt *epkt;
	struct	netpacket *pkt;
	uint32 i;			/* Counts bytes during copy	*/
	int32	src, dst;
	uint32	cksum;
	uint16	cksum16;
	uint16	*ptr16;

	ethptr = &ethertab[devptr->dvminor];

	csrptr = (struct eth_q_csreg *)ethptr->csr;

	/* Wait for an empty slot in the transmit descriptor ring */

	wait(ethptr->osem);

	/* Point to the tail of the descriptor ring */

	descptr = (struct eth_q_tx_desc *)ethptr->txRing + ethptr->txTail;

	/* Increment the tail index and wrap, if needed */

	ethptr->txTail += 1;
	if(ethptr->txTail >= ethptr->txRingSize) {
		ethptr->txTail = 0;
	}

	/* Add packet length to the descriptor */

	descptr->buf1size = 14 + len;

	epkt = (struct etherPkt *)descptr->buffer1;
	pkt = (struct netpacket *)buf;
	dst = pkt->net_raddstaddr[7];
	src = pkt->net_radsrcaddr[7];
	/*epkt->ipvh = 0x45;
	epkt->iptos = 0;
	epkt->iplen = htons(20+len);
	epkt->mbz1 = 0;
	epkt->mbz2 = htons(0xff00);
	epkt->ipcksum = 0;
	epkt->ipsrc = htonl(0x800a0300+src);
	epkt->ipdst = htonl(0x800a0300+dst);*/
	byte bcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	if(!memcmp(pkt->net_raddstaddr, bcast, 8)) {
		memset(epkt->dst, 0xff, 6);
	}
	else {
		//memcpy(epkt->dst, xinube_macs[dst-101], 6);
		memcpy(epkt->dst, xinube_macs[beserver], 6);
	}
	memcpy(epkt->src, ethptr->devAddress, 6);
	epkt->type = htons(0);
	epkt->dstbe = dst;
	epkt->srcbe = src;
	/*cksum = 0;
	ptr16 = (uint16 *)&epkt->ipvh;
	for(i = 0; i < 10; i++) {
		cksum += htons(*ptr16);
		ptr16++;
	}
	cksum = (cksum&0xffff)+(cksum>>16);
	cksum16 = (uint16)cksum;
	cksum16 = ~cksum16;
	epkt->ipcksum = htons(cksum16);*/

	/* Copy packet into the buffer associated with the descriptor	*/

	for(i = 0; i < len; i++) {
		*((char *)epkt->data + i) = *(buf + i);
	}

	for(i = 0; i < 50; i++) {
		kprintf("%02x ", *((char *)epkt + i));
	}
	kprintf("\n");

	/* Mark the descriptor if we are at the end of the ring */

	if(ethptr->txTail == 0) {
		descptr->ctrlstat = ETH_QUARK_TDCS_TER;
	} else {
		descptr->ctrlstat = 0;
	}

	/* Initialize the descriptor */

	descptr->ctrlstat |=
		(ETH_QUARK_TDCS_OWN | /* The desc is owned by DMA	*/
		 ETH_QUARK_TDCS_IC  | /* Interrupt after transfer	*/
		 ETH_QUARK_TDCS_LS  | /* Last segment of packet		*/
		 ETH_QUARK_TDCS_FS);  /* First segment of packet	*/

	/* Un-suspend DMA on the device */

	csrptr->tpdr = 1;

	return OK;
}
