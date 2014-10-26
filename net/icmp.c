/* icmp.c - icmp_register, icmp_in, icmp_recv */

#include <xinu.h>

struct	icmpentry icmptab[ICMP_SLOTS];

/*------------------------------------------------------------------------
 * icmp_register  -  Registers a slot for incoming ICMP packets
 *------------------------------------------------------------------------
 */
int32	icmp_register (
	byte	icremip[16],	/* Remote IP address	*/
	byte	iclocip[16],	/* Local IP address	*/
	byte	ictype,		/* ICMP Type		*/
	byte	iccode		/* ICMP Code		*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to ICMP table entry	*/
	intmask	mask;			/* Saved interrupt mask		*/
	int32	found;			/* Free index in ICMP table	*/
	int32	i;			/* Index in ICMP table		*/

	/* Ensure only one process accesses ICMP table */

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

		if( (!memcmp(icremip, icptr->icremip, 16)) &&
		    (!memcmp(iclocip, icptr->iclocip, 16)) &&
		    (ictype == icptr->ictype) &&
		    (iccode == icptr->iccode) ) {
		    	restore(mask);
			return SYSERR;
		}
	}

	if(found == -1) {
		restore(mask);
		return SYSERR;
	}

	icptr = &icmptab[found];
	memcpy(icptr->icremip, icremip, 16);
	memcpy(icptr->iclocip, iclocip, 16);
	icptr->ictype = ictype;
	icptr->iccode = iccode;
	icptr->icpid = (pid32)-1;

	icptr->ichead = icptr->ictail = 0;
	icptr->iccount = 0;

	icptr->icstate = ICMP_USED;

	restore(mask);
	return found;
}

/*------------------------------------------------------------------------
 * icmp_release  -  Release a slot in icmp table
 *------------------------------------------------------------------------
 */
int32	icmp_release (
	int32	slot	/* Slot to release	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to icmp table entry	*/
	intmask	mask;			/* Saved interrupt mask		*/

	if( (slot < 0) || (slot >= ICMP_SLOTS) ) {
		return SYSERR;
	}

	/* Ensure only one process accesses icmp table */

	mask = disable();

	icptr = &icmptab[slot];

	if(icptr->icstate == ICMP_FREE) {
		restore(mask);
		return SYSERR;
	}

	while(icptr->iccount > 0) {
		freebuf((char *)icptr->icqueue[icptr->ichead]);
		icptr->ichead += 1;
		if(icptr->ichead >= ICMP_QSIZ) {
			icptr->ichead = 0;
		}
		icptr->iccount--;
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
	struct	netpacket *pkt	/* Incoming packet	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to ICMP table entry	*/
	intmask	mask;			/* Saved interrupt mask		*/
	int32	found;			/* Matched icmp table index	*/
	int32	i;			/* Index in ICMP table		*/

	/*if(pkt->net_ictype == ICMP_ECHOREQST) {
		//kprintf("icmp_in echo request\n");
		memcpy(pkt->net_ipdst, pkt->net_ipsrc, 16);
		memcpy(pkt->net_ipsrc, if_tab[0].if_ipucast[0].ipaddr, 16);
		pkt->net_ictype = ICMP_ECHOREPLY;
		ip_enqueue(pkt);
		return;
	}*/

	/* Ensure only one process accesses ICMP table */

	mask = disable();

	found = -1;
	for(i = 0; i < ICMP_SLOTS; i++) {

		icptr = &icmptab[i];

		if(icptr->icstate == ICMP_FREE) {
			continue;
		}

		if( (!memcmp(icptr->icremip, ipunspec, 16)) &&
		    (!memcmp(icptr->iclocip, pkt->net_ipdst, 16)) &&
		    (icptr->ictype == pkt->net_ictype) &&
		    (icptr->iccode == pkt->net_iccode) ) {
		    	found = i;
			continue;
		}

		if( (!memcmp(icptr->icremip, pkt->net_ipsrc, 16)) &&
		    (!memcmp(icptr->iclocip, pkt->net_ipdst, 16)) &&
		    (icptr->ictype == pkt->net_ictype) &&
		    (icptr->iccode == pkt->net_iccode) ) {
			found = i;
			break;
		}
	}

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

		restore(mask);

		if((icptr->iccount == 1) && ((int32)icptr->icpid != -1)) {
			send(icptr->icpid, (umsg32)found);
		}

		return;
	}

	freebuf((char *)pkt);
	restore(mask);
}

/*------------------------------------------------------------------------
 * icmp_recv  -  Receive data in an incoming ICMP packet
 *------------------------------------------------------------------------
 */
int32	icmp_recv (
	int32	slot,	/* Slot in ICMp table	*/
	char	*buf,	/* Buffer to store data	*/
	uint32	len,	/* Size of the data	*/
	uint32	timeout	/* Timeout in millisecs	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to ICMP table entry	*/
	struct	netpacket *pkt;		/* Pointer to packet		*/
	umsg32	msg;			/* Message received		*/
	intmask	mask;			/* Saved interrupt mask		*/
	uint32	iclen;			/* ICMP data length		*/

	if( (slot < 0) || (slot >= ICMP_SLOTS) ) {
		return SYSERR;
	}

	/* Ensure only one process accesses icmp table */

	mask = disable();

	icptr = &icmptab[slot];

	if(icptr->icstate == ICMP_FREE) {
		restore(mask);
		return SYSERR;
	}

	if(icptr->iccount == 0) {
		icptr->icpid = getpid();
		msg = recvtime(timeout);
		if((int32)msg == TIMEOUT) {
			restore(mask);
			return TIMEOUT;
		}
		if(msg != (umsg32)slot) {
			restore(mask);
			return SYSERR;
		}
		icptr->icpid = -1;
	}

	pkt = icptr->icqueue[icptr->ichead];
	icptr->ichead++;
	if(icptr->ichead >= ICMP_QSIZ) {
		icptr->ichead = 0;
	}
	icptr->iccount--;

	iclen = pkt->net_iplen - ICMP_HDR_LEN;
	if(iclen > len) {
		iclen = len;
	}

	memcpy(buf, (char *)pkt->net_icdata, iclen);

	freebuf((char *)pkt);

	restore(mask);
	return iclen;
}

/*------------------------------------------------------------------------
 * icmp_recvfrom  -  Receive data in an incoming ICMP packet
 *------------------------------------------------------------------------
 */
int32	icmp_recvfrom (
	int32	slot,	/* Slot in ICMp table	*/
	byte	*ipsrc,	/* Source IP of packet	*/
	char	*buf,	/* Buffer to store data	*/
	uint32	len,	/* Size of the data	*/
	uint32	timeout	/* Timeout in millisecs	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to ICMP table entry	*/
	struct	netpacket *pkt;		/* Pointer to packet		*/
	umsg32	msg;			/* Message received		*/
	intmask	mask;			/* Saved interrupt mask		*/
	uint32	iclen;			/* ICMP data length		*/

	if( (slot < 0) || (slot >= ICMP_SLOTS) ) {
		return SYSERR;
	}

	/* Ensure only one process accesses icmp table */

	mask = disable();

	icptr = &icmptab[slot];

	if(icptr->icstate == ICMP_FREE) {
		restore(mask);
		return SYSERR;
	}

	if(icptr->iccount == 0) {
		icptr->icpid = getpid();
		msg = recvtime(timeout);
		if((int32)msg == TIMEOUT) {
			restore(mask);
			return TIMEOUT;
		}
		if(msg != (umsg32)slot) {
			restore(mask);
			return SYSERR;
		}
		icptr->icpid = (pid32)-1;
	}

	pkt = icptr->icqueue[icptr->ichead];
	icptr->ichead++;
	if(icptr->ichead >= ICMP_QSIZ) {
		icptr->ichead = 0;
	}
	icptr->iccount--;

	iclen = pkt->net_iplen - ICMP_HDR_LEN;
	if(iclen > len) {
		iclen = len;
	}

	memcpy((char *)ipsrc, pkt->net_ipsrc, 16);

	memcpy(buf, (char *)pkt->net_icdata, iclen);

	freebuf((char *)pkt);

	restore(mask);
	return iclen;
}

/*------------------------------------------------------------------------
 * icmp_send  -  Send an ICMP packet
 *------------------------------------------------------------------------
 */
int32	icmp_send (
	int32	slot,	/* Slot in ICMP table	*/
	char	*buf,	/* Buffer pointer	*/
	uint32	len	/* Length of data	*/
	)
{
	struct	icmpentry *icptr;	/* Pointer to ICMP table entry	*/
	struct	netpacket *pkt;		/* Pointer to packet		*/
	intmask	mask;			/* Saved interrupt mask		*/

	if( (slot < 0) || (slot >= ICMP_SLOTS) ) {
		return SYSERR;
	}

	/* Ensure only one process can access icmp table */

	mask = disable();

	icptr = &icmptab[slot];

	if(icptr->icstate == ICMP_FREE) {
		restore(mask);
		return SYSERR;
	}

	pkt = (struct netpacket *)getbuf(netbufpool);
	if((int32)pkt == SYSERR) {
		restore(mask);
		return SYSERR;
	}

	//memcpy(pkt->net_pansrc, if_tab[0].if_eui64, 8);
	//memcpy(pkt->net_pandst, &icptr->icremip[8], 8);
	pkt->net_ipvtch = 0x60;
	pkt->net_iphl = 255;
	pkt->net_ipnh = IP_ICMP;
	pkt->net_iplen = ICMP_HDR_LEN + len;
	memcpy(pkt->net_ipsrc, icptr->iclocip, 16);
	memcpy(pkt->net_ipdst, icptr->icremip, 16);
	pkt->net_ictype = icptr->ictype;
	pkt->net_iccode = icptr->iccode;

	restore(mask);

	memcpy(pkt->net_icdata, buf, len);

	//write(ETHER0, (char *)pkt, len+4+40+22);
	ip_send(pkt);

	return len;
}

/*------------------------------------------------------------------------
 * icmp_echoproc  -  Process that handles ICMP echo requests
 *------------------------------------------------------------------------
 */
process	icmp_echoproc (void)
{
	int32	recvslot;	/* Slot in ICMP table	*/
	int32	sendslot;	/* Slot in ICMP table	*/
	byte	ipsrc[16];	/* Source IP address	*/
	char	buf[56];	/* Buffer to store data	*/
	int32	retval;		/* Return value		*/

	recvslot = icmp_register(ipunspec, if_tab[0].if_ipucast[0].ipaddr, ICMP_ECHOREQST, 0);
	if(recvslot == SYSERR) {
		panic("ICMP echo process ICMP registration failed\n");
	}

	while(TRUE) {

		retval = icmp_recvfrom(recvslot, ipsrc, buf, 56, 300000);
		if(retval == SYSERR) {
			panic("ICMP echo process icmp_recv error\n");
		}
		else if(retval == TIMEOUT) {
			continue;
		}

		sendslot = icmp_register(ipsrc, if_tab[0].if_ipucast[0].ipaddr, ICMP_ECHOREPLY, 0);
		if(sendslot == SYSERR) {
			continue;
		}

		icmp_send(sendslot, buf, retval);

		icmp_release(sendslot);
	}

	return OK;
}

/*------------------------------------------------------------------------
 * icmp_init  -  Initialize the ICMP table
 *------------------------------------------------------------------------
 */
void	icmp_init ()
{
	int32	i;	/* Index in ICMP table	*/
	intmask	mask;	/* Saved interrupt mask	*/

	/* Ensure only one process accesses ICMP table */

	mask = disable();

	for(i = 0; i < ICMP_SLOTS; i++) {
		icmptab[i].icstate = ICMP_FREE;
	}

	restore(mask);
}
