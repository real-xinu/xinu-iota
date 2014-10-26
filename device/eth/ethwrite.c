/* ethwrite.c - ethwrite */

#include <xinu.h>

uint16	ipcksum(struct pradpacket *);
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
	struct	pradpacket *pktptr;
	struct	netpacket  *npktptr;
	uint32 i;			/* Counts bytes during copy	*/
	int32	dest;

	ethptr = &ethertab[devptr->dvminor];

	csrptr = (struct eth_q_csreg *)ethptr->csr;

	npktptr = (struct netpacket *)buf;
	dest = npktptr->net_pandst[7] - 101;
	if((dest < 0) || (dest > 95)) {
		return SYSERR;
	}

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

	descptr->buf1size = len + 20 + 14;

	/* Add the ethernet and IPv4 headers */

	pktptr = (struct pradpacket *)descptr->buffer1;

	if(NetData.coord) {
		memcpy((char *)pktptr->prad_ethdst, (char *)xinube_macs[0], 6);
	}
	else {
		memcpy((char *)pktptr->prad_ethdst, (char *)xinube_macs[dest], 6);
	}
	memcpy((char *)pktptr->prad_ethsrc, (char *)NetData.ethucast, 6);
	pktptr->prad_ethtype = htons(0x0800);

	pktptr->prad_ipvh = 0x45;
	pktptr->prad_iptos = 0;
	pktptr->prad_iplen = htons(20 + len);
	pktptr->prad_ipid = 0;
	pktptr->prad_ipfrag = 0;
	pktptr->prad_ipttl = 2;
	pktptr->prad_ipproto = PRAD_IPPROTO;
	pktptr->prad_ipcksum = 0;
	pktptr->prad_ipsrc = htonl(NetData.ipucast);
	pktptr->prad_ipdst = 0x800a0300 + npktptr->net_pandst[7];
	pktptr->prad_ipdst = htonl(pktptr->prad_ipdst);

	pktptr->prad_ipcksum = htons(ipcksum(pktptr));

	/* Copy packet into the buffer associated with the descriptor	*/

	for(i = 0; i < len; i++) {
		*((char *)pktptr->prad_data + i) = *((char *)buf + i);
	}
	/*for(i = 0; i < 100; i++) {
		kprintf("%02x ", *((char *)pktptr + i));
	}
	kprintf("\n");*/
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
uint16	ipcksum(
	 struct  pradpacket *pkt		/* Pointer to the packet	*/
	)
{
	uint16	*hptr;			/* Ptr to 16-bit header values	*/
	int32	i;			/* Counts 16-bit values in hdr	*/
	uint16	word;			/* One 16-bit word		*/
	uint32	cksum;			/* Computed value of checksum	*/

	hptr= (uint16 *) &pkt->prad_ipvh;

	/* Sum 16-bit words in the packet */

	cksum = 0;
	for (i=0; i<10; i++) {
		word = *hptr++;
		cksum += (uint32) htons(word);
	}

	/* Add in carry, and take the ones-complement */

	cksum += (cksum >> 16);
	cksum = 0xffff & ~cksum;

	/* Use all-1s for zero */

	if (cksum == 0xffff) {
		cksum = 0;
	}
	return (uint16) (0xffff & cksum);
}

