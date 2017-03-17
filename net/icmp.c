/* icmp.c - icmp_in */

#include <xinu.h>

struct	icentry icmptab[ICMP_TABSIZE];
/*------------------------------------------------------------------------
 * icmp_in  -  Handle incoming ICMP packets
 *------------------------------------------------------------------------
 */
void	icmp_in (
		struct	netpacket *pkt	/* Packet buffer pointer	*/
		)
{
	struct	icentry *icptr;
	struct	netpacket *newpkt;
	intmask	mask;
	int32	i;

	#ifdef DEBUG_ICMP
	kprintf("icmp_in: type %d code %d src: ", pkt->net_ictype, pkt->net_iccode); ip_printaddr(pkt->net_ipsrc);
	kprintf(" dst: "); ip_printaddr(pkt->net_ipdst); kprintf("\n");
	#endif

	/* Verify ICMP checksum */
	if(icmp_cksum(pkt) != 0) {
		//return;
	}

	switch(pkt->net_ictype) {

	case ICMP_TYPE_ERQ:

		#ifdef DEBUG_ICMP
		kprintf("icmp_in: ERQ from :"); ip_printaddr(pkt->net_ipsrc); kprintf("\n");
		#endif
		icmp_send(ICMP_TYPE_ERP, 0, pkt->net_ipdst, pkt->net_ipsrc, pkt->net_icdata,
				pkt->net_iplen - 4, pkt->net_iface);
		return;

	case ICMP_TYPE_NS:
	case ICMP_TYPE_NA:
		nd_in(pkt);
		return;

	case ICMP_TYPE_RPL:
		rpl_in(pkt);
		return;

	default:
		break;

	}

	mask = disable();
	for(i = 0; i < ICMP_TABSIZE; i++) {

		icptr = &icmptab[i];

		if(icptr->icstate == ICENTRY_FREE) {
			continue;
		}

		#ifdef DEBUG_ICMP
		kprintf("icmp_in: slot: t %d c %d remip ", icptr->ictype, icptr->iccode);
		ip_printaddr(icptr->icremip); kprintf(" locip "); ip_printaddr(icptr->iclocip);
		kprintf(" iface %d\n", icptr->iciface);
		#endif
		if( (icptr->ictype == pkt->net_ictype) &&
		    (icptr->iccode == pkt->net_iccode) &&
		    (isipunspec(icptr->icremip) || !memcmp(icptr->icremip, pkt->net_ipsrc, 16)) &&
		    (isipunspec(icptr->iclocip) || !memcmp(icptr->iclocip, pkt->net_ipdst, 16)) &&
		    (icptr->iciface == -1 || icptr->iciface == pkt->net_iface) ) {
			break;
		}
	}

	if(i >= ICMP_TABSIZE) {
		#ifdef DEBUG_ICMP
		kprintf("icmp_in: no matching slot\n");
		#endif
		restore(mask);
		return;
	}

	if(icptr->iccount >= ICENTRY_PQSIZE) {
		restore(mask);
		return;
	}

	newpkt = (struct netpacket *)getbuf(netbufpool);
	if((int32)newpkt == SYSERR) {
		restore(mask);
		return;
	}

	memcpy(newpkt, pkt, 40 + pkt->net_iplen);

	icptr->icpktq[icptr->ictail++] = newpkt;
	if(icptr->ictail >= ICENTRY_PQSIZE) {
		icptr->ictail = 0;
	}
	icptr->iccount++;

	if(icptr->icstate == ICENTRY_RECV) {
		send(icptr->icpid, OK);
	}
}

/*------------------------------------------------------------------------
 * icmp_send  -  Send an ICMP packet
 *------------------------------------------------------------------------
 */
int32	icmp_send (
		byte	ictype,		/* ICMP type		*/
		byte	iccode,		/* ICMP code		*/
		byte	ipsrc[],	/* IP source		*/
		byte	ipdst[],	/* IP destination	*/
		void	*icdata,	/* ICMP data		*/
		int32	datalen,	/* ICMP data length	*/
		int32	iface		/* Interface index	*/
		)
{
	struct	netpacket *pkt;	/* Packet buffer pointer	*/

	#ifdef DEBUG_ICMP
	kprintf("icmp_send: type %d, code: %d, dst: ", ictype, iccode); ip_printaddr(ipdst);
	kprintf("\n");
	#endif

	/* Allocate buffer for the packet */
	pkt = (struct netpacket *)getbuf(netbufpool);
	if((int32)pkt == SYSERR) {
		return SYSERR;
	}

	/* If Ip dest is link-local, an interface must be provided */
	if(isipllu(ipdst) && iface == -1) {
		return SYSERR;
	}

	memset(pkt, 0, PACKLEN);

	/* Set the IP, ICMP fields in the packet */
	pkt->net_iface = iface;

	pkt->net_ipvtch = 0x60;
	pkt->net_ipnh = IP_ICMP;
	pkt->net_iphl = 255;
	pkt->net_iplen = 4 + datalen;
	memcpy(pkt->net_ipsrc, ipsrc, 16);
	memcpy(pkt->net_ipdst, ipdst, 16);

	pkt->net_ictype = ictype;
	pkt->net_iccode = iccode;
	memcpy(pkt->net_icdata, icdata, datalen);

	ip_send(pkt);

	return OK;
}

/*------------------------------------------------------------------------
 * icmp_register  -  Register a slot in the ICMP table
 *------------------------------------------------------------------------
 */
int32	icmp_register (
		byte	ictype,
		byte	iccode,
		byte	icremip[],
		byte	iclocip[],
		int32	iciface
		)
{
	struct	icentry *icptr;
	intmask	mask;
	int32	found = -1;
	int32	i;

	mask = disable();

	for(i = 0; i < ICMP_TABSIZE; i++) {

		icptr = &icmptab[i];

		if(icptr->icstate == ICENTRY_FREE) {
			found = found == -1 ? i : found;
			continue;
		}

		if( (icptr->ictype == ictype) &&
		    (icptr->iccode == iccode) &&
		    (isipunspec(icptr->icremip) || !memcmp(icptr->icremip, icremip, 16) || isipunspec(icremip)) &&
		    (isipunspec(icptr->iclocip) || !memcmp(icptr->iclocip, iclocip, 16) || isipunspec(iclocip)) &&
		    (icptr->iciface == -1 || icptr->iciface == iciface || iciface == -1) ) {
			restore(mask);
			return SYSERR;
		}
	}

	if(found == -1) {
		restore(mask);
		return SYSERR;
	}

	icptr = &icmptab[found];

	icptr->ictype = ictype;
	icptr->iccode = iccode;
	memcpy(icptr->icremip, icremip, 16);
	memcpy(icptr->iclocip, iclocip, 16);
	icptr->iciface = iciface;
	icptr->ichead = 0;
	icptr->ictail = 0;
	icptr->iccount = 0;
	icptr->icstate = ICENTRY_USED;

	restore(mask);
	return found;
}

/*------------------------------------------------------------------------
 * icmp_release  -  Release a slot from ICMP table
 *------------------------------------------------------------------------
 */
int32	icmp_release (
		int32	slot
		)
{
	struct	icentry *icptr;
	intmask	mask;

	if(slot < 0 || slot >= ICMP_TABSIZE) {
		return SYSERR;
	}

	mask = disable();

	icptr = &icmptab[slot];

	if(icptr->icstate == ICENTRY_FREE) {
		restore(mask);
		return SYSERR;
	}

	if(icptr->icstate == ICENTRY_RECV) {
		send(icptr->icpid, SYSERR);
	}

	while(icptr->iccount > 0) {
		freebuf((char *)icptr->icpktq[icptr->ichead++]);
		if(icptr->ichead >= ICENTRY_PQSIZE) {
			icptr->ichead = 0;
		}
		icptr->iccount--;
	}

	icptr->icstate = ICENTRY_FREE;

	restore(mask);
	return OK;
}

/*------------------------------------------------------------------------
 * icmp_recv  -  Receive an ICMP packet on specified ICMP slot
 *------------------------------------------------------------------------
 */
int32	icmp_recv (
		int32	icslot,
		char	data[],
		int32	datalen,
		uint32	timeout
		)
{
	struct	icentry *icptr;
	struct	netpacket *pkt;
	intmask	mask;
	int32	retval;

	mask = disable();

	icptr = &icmptab[icslot];

	if(icptr->icstate == ICENTRY_FREE) {
		restore(mask);
		return SYSERR;
	}

	if(icptr->iccount == 0) {
		icptr->icstate = ICENTRY_RECV;
		icptr->icpid = getpid();
		retval = recvtime(timeout);
		if(retval == SYSERR || retval == TIMEOUT) {
			icptr->icstate = ICENTRY_USED;
			restore(mask);
			return retval;
		}
	}

	pkt = icptr->icpktq[icptr->ichead++];
	if(icptr->ichead >= ICENTRY_PQSIZE) {
		icptr->ichead = 0;
	}
	icptr->iccount--;

	if((pkt->net_iplen - 4) < datalen) {
		datalen = pkt->net_iplen - 4;
	}

	memcpy(data, pkt->net_icdata, datalen);

	freebuf((char *)pkt);
	restore(mask);

	return datalen;
}

/*------------------------------------------------------------------------
 * icmp_cksum  -  Compute checksum of an ICMP packet
 *------------------------------------------------------------------------
 */
uint16	icmp_cksum (
		struct	netpacket *pkt	/* Packet buffer pointer	*/
		)
{
	uint32	cksum;	/* 32-bit sum of all shorts	*/
	uint16	*ptr16;	/* Pointer to a short		*/
	int32	i;	/* Index variable		*/

	/* This is the pseudo header */
	#pragma pack(1)
	struct	{
		byte	ipsrc[16];
		byte	ipdst[16];
		uint32	icmplen;
		byte	zero[3];
		byte	ipnh;
	} pseudo;
	#pragma pack()

	/* Initialize the pseudo header */
	memset(&pseudo, 0, sizeof(pseudo));
	memcpy(pseudo.ipsrc, pkt->net_ipsrc, 16);
	memcpy(pseudo.ipdst, pkt->net_ipdst, 16);
	pseudo.icmplen = htonl(pkt->net_iplen);
	pseudo.ipnh = IP_ICMP;

	cksum = 0;
	ptr16 = (uint16 *)&pseudo;

	/* First add all shorts in the pseudo header */
	for(i = 0; i < sizeof(pseudo); i = i + 2) {
		cksum += htons(*ptr16);
		ptr16++;
	}

	ptr16 = (uint16 *)pkt->net_ipdata;

	/* Add all the shorts in the ICMP packet */
	for(i = 0; i < pkt->net_iplen; i = i + 2) {
		cksum += htons(*ptr16);
		ptr16++;
	}

	cksum = (uint16)cksum + (cksum >> 16);

	return (uint16)~cksum;
}
