/* icmp.c - icmp_init, icmp_in, icmp_recv, icmp_recvaddr, icmp_send */

#include <xinu.h>

struct	icmpentry icmptab[ICMP_SLOTS];
uint16	icmp_cksum(struct netpacket *);
int32	icmp_send(int32, byte, byte, struct ipinfo *, char *, int32);

/*------------------------------------------------------------------------
 * icmp_init  -  Initialize the ICMP table
 *------------------------------------------------------------------------
 */
void	icmp_init (void)
{
	struct	icmpentry *icptr;	/* Pointer to ICMP entry	*/
	intmask	mask;			/* Saved interrupt mask		*/
	int32	i;			/* For loop index		*/

	/* Ensure only one process accesses the ICMP table */

	mask = disable();

	for(i = 0; i < ICMP_SLOTS; i++) {

		/* Initialize the table entry */

		icptr = &icmptab[i];
		icptr->icstate = ICMP_FREE;
		icptr->ichead = icptr->ictail = 0;
		icptr->iccount = 0;
	}

	restore(mask);
}

/*------------------------------------------------------------------------
 * icmp_register  -  Register a slot in the ICMP table
 *------------------------------------------------------------------------
 */
int32	icmp_register (
	int32	iface,		/* Interface index	*/
	byte	remip[16],	/* Remote IP address	*/
	byte	locip[16],	/* Local IP address	*/
	byte	type,		/* ICMP type		*/
	byte	code		/* ICMP code		*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to icmp table entry	*/
	intmask	mask;			/* Saved interrupt mask		*/
	int32	found;			/* Empty slot in icmp table	*/
	int32	i;			/* For loop index		*/

	mask = disable();

	found = -1;
	for(i = 0; i < ICMP_SLOTS; i++) {
		icptr = &icmptab[i];

		if(icptr->icstate == ICMP_FREE) {
			if(found == -1) {
				found = i;
			}
			continue;
		}

		if((icptr->iciface == iface) &&
		   (icptr->ictype == type) &&
		   (icptr->iccode == code)) {

			if(!memcmp(icptr->icremip, remip, 16)) {
				return SYSERR;
			}
			/*
		   	if((!memcmp(icptr->iclocip, ip_unspec, 16)) ||
			   (!memcmp(locip, ip_unspec, 16)) ||
			   (!memcmp(icptr->iclocip, locip, 16))) {

				if((!memcmp(icptr->icremip, ip_unspec, 16)) ||
				   (!memcmp(remip, ip_unspec, 16)) ||
				   (!memcmp(icptr->icremip, remip, 16))) {
				   	restore(mask);
					return SYSERR;
				}
			}
			*/
		}
	}

	if(found == -1) {
		restore(mask);
		return SYSERR;
	}

	icptr = &icmptab[found];

	icptr->iciface = iface;
	memcpy(icptr->iclocip, locip, 16);
	memcpy(icptr->icremip, remip, 16);
	icptr->ictype = type;
	icptr->iccode = code;
	icptr->ichead = icptr->ictail = 0;
	icptr->iccount = 0;
	icptr->icstate = ICMP_USED;

	restore(mask);
	return found;
}

/*------------------------------------------------------------------------
 * icmp_release  -  Release a slot in ICMP table
 *------------------------------------------------------------------------
 */
int32	icmp_release (
	int32	slot	/* Slot to be released	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to icmp table entry	*/
	struct	netpacket *pkt;		/* Pointer to packet buffer	*/
	intmask	mask;			/* Saved interrupt mask		*/
	int32	i;			/* For loop index		*/

	if((slot < 0) || (slot >= ICMP_SLOTS)) {
		return SYSERR;
	}

	mask = disable();

	icptr = &icmptab[slot];

	if(icptr->icstate == ICMP_FREE) {
		restore(mask);
		return SYSERR;
	}

	if(icptr->icstate == ICMP_RECV) {
		send(icptr->icpid, SYSERR);
	}

	for(i = 0; i < icptr->iccount; i++) {
		pkt = icptr->icqueue[icptr->ichead];
		freebuf((char *)pkt);
		icptr->ichead++;
		if(icptr->ichead >= ICMP_QSIZ) {
			icptr->ichead = 0;
		}
	}

	icptr->icstate = ICMP_FREE;

	restore(mask);
	return OK;
}

/*------------------------------------------------------------------------
 * icmp_in  -  Handle incoming ICMP packets
 *------------------------------------------------------------------------
 */
void	icmp_in (
	struct	netpacket *pkt	/* Pointer to packet	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to icmp table entry	*/
	struct	ipinfo ipdata;		/* IP information struct	*/
	intmask	mask;			/* Saved interrupt mask		*/
	int32	found;			/* Index of matching ICMP entry	*/
	int32	i;			/* For loop index		*/

	kprintf("icmp_in: type %d code %d\n", pkt->net_ictype, pkt->net_iccode);
	mask = disable();
	/*
	if((pkt->net_ictype == ICMP_ECHOREQST) && (pkt->net_iccode == 0)) {
		memcpy(ipdata.ipdst, pkt->net_ipsrc, 16);
		memcpy(ipdata.ipsrc, ip_unspec, 16);
		ipdata.iphl = 255;
		icmp_send(pkt->net_iface, ICMP_ECHOREPLY, 0, &ipdata, pkt->net_icdata, pkt->net_iplen-ICMP_HDR_LEN);
		freebuf((char *)pkt);
		restore(mask);
		return;
	}
	*/
	found = -1;
	for(i = 0; i < ICMP_SLOTS; i++) {
		icptr = &icmptab[i];

		if(icptr->icstate == ICMP_FREE) {
			continue;
		}

		if((icptr->iciface == pkt->net_iface) &&
		   (icptr->ictype == pkt->net_ictype) &&
		   (icptr->iccode == pkt->net_iccode)) {
			/*
		   	if(memcmp(icptr->iclocip, ip_unspec, 16)) {
				if(memcmp(icptr->iclocip, pkt->net_ipdst, 16)) {
					continue;
				}
			}
			*/

			if(!memcmp(icptr->icremip, ip_unspec, 16)) {
				found = i;
				continue;
			}

			if(!memcmp(icptr->icremip, pkt->net_ipsrc, 16)) {
				found = i;
				break;
			}
		}
	}

	/*
	if(memcmp(icptr->icremip, ip_unspec, 16)) {
		if(memcmp(icptr->icremip, pkt->net_ipsrc, 16)) {
					continue;
				}
			}
	*/

	if(found != -1) {
		icptr = &icmptab[found];

		if(icptr->iccount >= ICMP_QSIZ) {
			freebuf((char *)pkt);
			restore(mask);
			return;
		}

		icptr->icqueue[icptr->ictail] = pkt;
		icptr->ictail++;
		if(icptr->ictail >= ICMP_QSIZ) {
			icptr->ictail = 0;
		}
		icptr->iccount++;

		if((icptr->icstate == ICMP_RECV) &&
		   (icptr->iccount == 1)) {
		   	send(icptr->icpid, found);
		}

		restore(mask);
		return;
	}

	freebuf((char *)pkt);
	restore(mask);
}

/*------------------------------------------------------------------------
 * icmp_recv  -  Copy the data in incoming packet in user buffer
 *------------------------------------------------------------------------
 */
int32	icmp_recv (
	int32	slot,	/* Slot in icmp table	*/
	char	*buf,	/* Ptr to user buffer	*/
	uint32	len,	/* Size of buffer	*/
	uint32	timeout	/* Timeout in millisecs	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to icmp table entry	*/
	intmask	mask;			/* Saved interrupt mask		*/
	struct	netpacket *pkt;		/* Pointer to a packet buffer	*/
	umsg32	msg;			/* Message to be recvd.		*/

	if((slot < 0) || (slot >= ICMP_SLOTS)) {
		return SYSERR;
	}

	mask = disable();

	icptr = &icmptab[slot];

	if(icptr->icstate != ICMP_USED) {
		restore(mask);
		return SYSERR;
	}

	if(icptr->iccount == 0) {
		icptr->icpid = getpid();
		icptr->icstate = ICMP_RECV;
		recvclr();
		msg = (umsg32)SYSERR;
		while((int32)msg != slot) {
			msg = recvtime(timeout);
			if((int32)msg == SYSERR) {
				restore(mask);
				return SYSERR;
			}
			if((int32)msg == TIMEOUT) {
				restore(mask);
				return TIMEOUT;
			}
		}
		icptr->icstate = ICMP_USED;
	}

	pkt = icptr->icqueue[icptr->ichead];
	icptr->ichead++;
	if(icptr->ichead >= ICMP_QSIZ) {
		icptr->ichead = 0;
	}
	icptr->iccount--;

	if(len > (pkt->net_iplen - ICMP_HDR_LEN)) {
		len = pkt->net_iplen - ICMP_HDR_LEN;
	}

	memcpy(buf, (char *)pkt->net_icdata, len);

	restore(mask);
	return len;
}

/*------------------------------------------------------------------------
 * icmp_recvn  -  Copy the data in incoming packet in user buffer
 * 		  wait on multiple slots
 *------------------------------------------------------------------------
 */
int32	icmp_recvn (
	int32	slots[],/* Slot in icmp table	*/
	uint32	nslots,	/* No. of slots		*/
	char	*buf,	/* Ptr to user buffer	*/
	uint32	*len,	/* Size of buffer	*/
	uint32	timeout	/* Timeout in millisecs	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to icmp table entry	*/
	intmask	mask;			/* Saved interrupt mask		*/
	struct	netpacket *pkt;		/* Pointer to a packet buffer	*/
	umsg32	msg;			/* Message to be recvd.		*/
	int32	retval;			/* Return value			*/
	int32	i;			/* For loop index		*/

	mask = disable();

	for(i = 0; i < nslots; i++) {

		if((slots[i] < 0) || (slots[i] >= ICMP_SLOTS)) {
			restore(mask);
			return SYSERR;
		}
		icptr = &icmptab[slots[i]];

		if(icptr->icstate != ICMP_USED) {
			restore(mask);
			return SYSERR;
		}
	}

	for(i = 0; i < nslots; i++) {
		icptr = &icmptab[slots[i]];

		if(icptr->iccount > 0) {
			retval = i;
			break;
		}
		icptr->icstate = ICMP_RECV;
		icptr->icpid = getpid();
	}

	if(i >= nslots) {
		recvclr();
		msg = recvtime(timeout);
		if((int32)msg == SYSERR) {
			restore(mask);
			return SYSERR;
		}
		if((int32)msg == TIMEOUT) {
			restore(mask);
			return TIMEOUT;
		}
		retval = msg;
		icptr = &icmptab[msg];
	}

	pkt = icptr->icqueue[icptr->ichead];
	icptr->ichead++;
	if(icptr->ichead >= ICMP_QSIZ) {
		icptr->ichead = 0;
	}
	icptr->iccount--;

	if((*len) > (pkt->net_iplen - ICMP_HDR_LEN)) {
		*len = pkt->net_iplen - ICMP_HDR_LEN;
	}

	memcpy(buf, (char *)pkt->net_icdata, *len);

	for(i = 0; i < nslots; i++) {
		icmptab[slots[i]].icstate = ICMP_USED;
	}

	restore(mask);
	return retval;
}

/*------------------------------------------------------------------------
 * icmp_recvaddr  -  Copy the data from incoming packet in user buffer
 *------------------------------------------------------------------------
 */
int32	icmp_recvaddr (
	int32	slot,	/* Slot in icmp table	*/
	char	*buf,	/* Ptr to user buffer	*/
	uint32	len,	/* Size of buffer	*/
	uint32	timeout,/* Timeout in millisecs	*/
	struct	ipinfo *ipdata/* IP information	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to icmp table entry	*/
	intmask	mask;			/* Saved interrupt mask		*/
	struct	netpacket *pkt;		/* Pointer to a packet buffer	*/
	umsg32	msg;			/* Message to be recvd.		*/

	if((slot < 0) || (slot >= ICMP_SLOTS)) {
		return SYSERR;
	}

	mask = disable();

	icptr = &icmptab[slot];

	if(icptr->icstate != ICMP_USED) {
		restore(mask);
		return SYSERR;
	}

	if(icptr->iccount == 0) {
		icptr->icpid = getpid();
		icptr->icstate = ICMP_RECV;
		recvclr();
		msg = (umsg32)SYSERR;
		while((int32)msg != slot) {
			msg = recvtime(timeout);
			if((int32)msg == TIMEOUT) {
				restore(mask);
				return TIMEOUT;
			}
			if((int32)msg == SYSERR) {
				restore(mask);
				return SYSERR;
			}
		}
		icptr->icstate = ICMP_USED;
	}

	pkt = icptr->icqueue[icptr->ichead];
	icptr->ichead++;
	if(icptr->ichead >= ICMP_QSIZ) {
		icptr->ichead = 0;
	}
	icptr->iccount--;

	if(len > (pkt->net_iplen - ICMP_HDR_LEN)) {
		len = pkt->net_iplen - ICMP_HDR_LEN;
	}

	kprintf("icmp_recvaddr: copying %d bytes\n", len);
	memcpy(buf, (char *)pkt->net_icdata, len);
	ipdata->iphl = pkt->net_iphl;
	memcpy(ipdata->ipsrc, pkt->net_ipsrc, 16);
	memcpy(ipdata->ipdst, pkt->net_ipdst, 16);

	restore(mask);
	return len;
}

/*------------------------------------------------------------------------
 * icmp_recvnaddr  -  Copy the data in incoming packet in user buffer
 * 		  wait on multiple slots
 *------------------------------------------------------------------------
 */
int32	icmp_recvnaddr (
	int32	slots[],/* Slot in icmp table	*/
	uint32	nslots,	/* No. of slots		*/
	char	*buf,	/* Ptr to user buffer	*/
	uint32	*len,	/* Size of buffer	*/
	uint32	timeout,/* Timeout in millisecs	*/
	struct	ipinfo *ipdata/* IP information	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to icmp table entry	*/
	intmask	mask;			/* Saved interrupt mask		*/
	struct	netpacket *pkt;		/* Pointer to a packet buffer	*/
	umsg32	msg;			/* Message to be recvd.		*/
	int32	retval;			/* Return value			*/
	int32	i;			/* For loop index		*/

	mask = disable();

	for(i = 0; i < nslots; i++) {

		if((slots[i] < 0) || (slots[i] >= ICMP_SLOTS)) {
			kprintf("icmp_recvnaddr: invalid slot[%d] %d\n", i, slots[i]);
			restore(mask);
			return SYSERR;
		}
		icptr = &icmptab[slots[i]];

		if(icptr->icstate != ICMP_USED) {
			kprintf("icmp_recvnaddr: slot[%d] state not used\n", i);
			restore(mask);
			return SYSERR;
		}
	}

	for(i = 0; i < nslots; i++) {
		icptr = &icmptab[slots[i]];

		if(icptr->iccount > 0) {
			retval = slots[i];
			break;
		}
		icptr->icstate = ICMP_RECV;
		icptr->icpid = getpid();
	}

	if(i >= nslots) {
		recvclr();
		msg = recvtime(timeout);
		if((int32)msg == SYSERR) {
			kprintf("icmp_recvnaddr: invalid msg revcd\n");
			for(i = 0; i < nslots; i++) {
				icmptab[slots[i]].icstate = ICMP_USED;
			}
			restore(mask);
			return SYSERR;
		}
		if((int32)msg == TIMEOUT) {
			for(i = 0; i < nslots; i++) {
				icmptab[slots[i]].icstate = ICMP_USED;
			}
			restore(mask);
			return TIMEOUT;
		}
		retval = msg;
		icptr = &icmptab[msg];
	}

	pkt = icptr->icqueue[icptr->ichead];
	icptr->ichead++;
	if(icptr->ichead >= ICMP_QSIZ) {
		icptr->ichead = 0;
	}
	icptr->iccount--;

	if((*len) > (pkt->net_iplen - ICMP_HDR_LEN)) {
		*len = pkt->net_iplen - ICMP_HDR_LEN;
	}

	memcpy(buf, (char *)pkt->net_icdata, *len);
	ipdata->iphl = pkt->net_iphl;
	memcpy(ipdata->ipsrc, pkt->net_ipsrc, 16);
	memcpy(ipdata->ipdst, pkt->net_ipdst, 16);

	for(i = 0; i < nslots; i++) {
		icmptab[slots[i]].icstate = ICMP_USED;
	}

	restore(mask);
	return retval;
}

/*------------------------------------------------------------------------
 * icmp_send  -  Send an ICMP packet
 *------------------------------------------------------------------------
 */
int32	icmp_send (
	int32	iface,	/* interface index	*/
	byte	type,	/* ICMP type		*/
	byte	code,	/* ICMP code		*/
	struct	ipinfo *ipdata,/* IP information*/
	char	*buf,	/* Data to be sent	*/
	int32	len	/* Length of data	*/
	)
{
	uint16	cksum;
	struct	netpacket *pkt;	/* Pointer to packet buffer	*/

	if((iface < 0) || (iface >= NIFACES)) {
		return SYSERR;
	}

	pkt = (struct netpacket *)getbuf(netbufpool);
	if((int32)pkt == SYSERR) {
		return SYSERR;
	}

	memset((char *)pkt, NULLCH, PACKLEN);

	pkt->net_ipvtch = 0x60;
	pkt->net_ipnh = IP_ICMPV6;
	pkt->net_iphl = ipdata->iphl;
	pkt->net_iplen = ICMP_HDR_LEN + len;
	memcpy(pkt->net_ipsrc, ipdata->ipsrc, 16);
	memcpy(pkt->net_ipdst, ipdata->ipdst, 16);

	pkt->net_ictype = type;
	pkt->net_iccode = code;
	pkt->net_iccksum = 0;

	memcpy(pkt->net_icdata, buf, len);

	cksum = icmp_cksum(pkt);
	pkt->net_iccksum = htons(cksum);

	pkt->net_iface = iface;

	kprintf("icmp_send: calling ip_send\n");
	ip_send(pkt);
	return OK;
}

/*------------------------------------------------------------------------
 * icmp_cksum  -  Compute ICMP checksum
 *------------------------------------------------------------------------
 */
uint16	icmp_cksum (
	struct	netpacket *pkt
	)
{
	uint32	sum;
	uint16	cksum, *ptr16;
	int32	icmplen, i;

	struct {
		byte	ipsrc[16];
		byte	ipdst[16];
		uint32	icmplen;
		byte	pad[3];
		byte	ipnh;
	} pseudo;

	icmplen = pkt->net_iplen;
	memcpy(pseudo.ipsrc, pkt->net_ipsrc, 16);
	memcpy(pseudo.ipdst, pkt->net_ipdst, 16);
	pseudo.icmplen = htonl(pkt->net_iplen);
	memset(pseudo.pad, 0, 3);
	pseudo.ipnh = IP_ICMPV6;

	sum = 0;
	ptr16 = (uint16 *)&pseudo;
	for(i = 0; i < 20; i++) {
		sum += htons(*ptr16);
		ptr16++;
	}

	if(icmplen%2) {
		pkt->net_ipdata[icmplen] = 0;
		icmplen++;
	}

	ptr16 = (uint16 *)&pkt->net_ictype;
	for(i = 0; i < (icmplen/2); i++) {
		sum += htons(*ptr16);
		ptr16++;
	}

	cksum = (sum&0xffff) + ((sum>>16)&0xffff);

	return (~cksum);
}
