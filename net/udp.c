/* udp.c - udp_init, udp_register, udp_recv, udp_recvaddr, udp_send */

#include <xinu.h>

struct	udpentry udptab[UDP_SLOTS];

/*------------------------------------------------------------------------
 * udp_init  -  Initialize the UDP table
 *------------------------------------------------------------------------
 */
void	udp_init (void) {

	struct	udpentry *udptr;	/* Pointer to UDP table entry	*/
	intmask	mask;			/* Saved interrupt mask		*/
	int32	i;			/* For loop index		*/

	mask = disable();

	for(i = 0; i < UDP_SLOTS; i++) {

		udptr = &udptab[i];
		memset(udptr, NULLCH, sizeof(struct udpentry));
		udptr->udstate = UDP_FREE;
	}

	restore(mask);
}

/*------------------------------------------------------------------------
 * udp_register  -  Register a slot in the UDP table
 *------------------------------------------------------------------------
 */
int32	udp_register (
	int32	iface,		/* Interface index	*/
	byte	remip[16],	/* Remotr IP address	*/
	uint16	remport,	/* Remote port		*/
	uint16	locport		/* Local port		*/
	)
{
	struct	udpentry *udptr;	/* Pointer to UDP table entry	*/
	intmask	mask;			/* Saved interrupt mask		*/
	int32	found = -1;		/* Empty slot in UDP table	*/
	int32	i;

	if((iface < 0) || (iface >= NIFACES)) {
		return SYSERR;
	}

	mask = disable();

	for(i = 0; i < UDP_SLOTS; i++) {

		udptr = &udptab[i];

		if(udptr->udstate == UDP_FREE) {
			found = found == -1 ? i : found;
			continue;
		}

		if((remport == udptr->udremport) &&
		   (locport == udptr->udlocport) &&
		   (iface == udptr->udiface)) {

			if(isipunspec(udptr->udremip)) {
				if(isipunspec(remip)) {
					restore(mask);
					return SYSERR;
				}
				continue;
			}

			if(!memcmp(udptr->udremip, remip, 16)) {
				restore(mask);
				return SYSERR;
			}
			continue;
		}
	}

	if(found == -1) {
		restore(mask);
		return SYSERR;
	}

	udptr = &udptab[found];
	udptr->udiface = iface;
	memcpy(udptr->udremip, remip, 16);
	udptr->udremport = remport;
	udptr->udlocport = locport;
	udptr->udhead = udptr->udtail = udptr->udcount = 0;
	udptr->udpid = -1;

	udptr->udstate = UDP_USED;

	restore(mask);
	return found;
}

/*------------------------------------------------------------------------
 * udp_in  -  Handle incoming UDP packets
 *------------------------------------------------------------------------
 */
void	udp_in (
	struct	netpacket *pkt
	)
{
	struct	udpentry *udptr;	/* Pointer in UDP table	*/
	intmask	mask;			/* Saved interrupt mask	*/
	int32	found = -1;		/* Empty slot in udptab	*/
	int32	i;			/* For loop index	*/

	mask = disable();

	for(i = 0; i < UDP_SLOTS; i++) {

		udptr = &udptab[i];
		if(udptr->udstate == UDP_FREE) {
			continue;
		}

		if((pkt->net_iface == udptr->udiface) &&
		   (pkt->net_udpsport == udptr->udremport) &&
		   (pkt->net_udpdport == udptr->udlocport)) {
			if(isipunspec(udptr->udremip)) {
				found = i;
				continue;
			}
			if(!memcmp(pkt->net_ipsrc, udptr->udremip, 16)) {
				found = i;
				break;
			}
		}
	}

	if(found == -1) {
		freebuf((char *)pkt);
		return;
	}

	udptr = &udptab[found];

	if(udptr->udcount >= UDP_QSIZ) {
		freebuf((char *)pkt);
		return;
	}

	udptr->udqueue[udptr->udtail] = pkt;
	udptr->udtail++;
	if(udptr->udtail >= UDP_QSIZ) {
		udptr->udtail = 0;
	}
	udptr->udcount++;

	if(udptr->udcount == 1) {
		if(udptr->udstate == UDP_RECV) {
			send(udptr->udpid, OK);
		}
	}

	restore(mask);
}

/*------------------------------------------------------------------------
 * udp_recv  -  Receive a packet on a UDP slot
 *------------------------------------------------------------------------
 */
int32	udp_recv (
	int32	slot,	/* Slot in UDP table		*/
	char	*buf,	/* User buffer to copy data	*/
	int32	len,	/* User buffer size		*/
	uint32	timeout	/* Timeout in ms		*/
	)
{
	struct	udpentry *udptr;	/* Pointer to UDP table	*/
	struct	netpacket *pkt;		/* Pointer to packet buf*/
	intmask	mask;			/* Saved interrupt mask	*/
	umsg32	msg;			/* Incoming message	*/
	int32	udplen;			/* Length of UDP data	*/

	if((slot < 0) || (slot >= UDP_SLOTS)) {
		return SYSERR;
	}

	mask = disable();

	udptr = &udptab[slot];

	if(udptr->udstate != UDP_USED) {
		restore(mask);
		return SYSERR;
	}

	if(udptr->udcount == 0) {
		udptr->udpid = getpid();
		udptr->udstate = UDP_RECV;
		recvclr();
		msg = recvtime(timeout);
		if((int32)msg == TIMEOUT) {
			udptr->udstate = UDP_USED;
			restore(mask);
			return TIMEOUT;
		}
		if((int32)msg != OK) {
			udptr->udstate = UDP_USED;
			restore(mask);
			return SYSERR;
		}
		udptr->udstate = UDP_USED;
	}

	pkt = udptr->udqueue[udptr->udhead];
	udptr->udhead++;
	if(udptr->udhead >= UDP_QSIZ) {
		udptr->udhead = 0;
	}
	udptr->udcount--;

	udplen = pkt->net_udplen - UDP_HDR_LEN;

	if(udplen < len) {
		len = udplen;
	}

	memcpy(buf, pkt->net_udpdata, len);

	freebuf((char *)pkt);
	restore(mask);
	return len;
}

/*------------------------------------------------------------------------
 * udp_recvaddr  -  Receive a packet on a UDP slot
 *------------------------------------------------------------------------
 */
int32	udp_recvaddr (
	int32	slot,	/* Slot in UDP table		*/
	char	*buf,	/* User buffer to copy data	*/
	int32	len,	/* User buffer size		*/
	uint32	timeout,/* Timeout in ms		*/
	struct	ipinfo *ipdata/* IP information		*/
	)
{
	struct	udpentry *udptr;	/* Pointer to UDP table	*/
	struct	netpacket *pkt;		/* Pointer to packet buf*/
	intmask	mask;			/* Saved interrupt mask	*/
	umsg32	msg;			/* Incoming message	*/
	int32	udplen;			/* UDP data length	*/

	if((slot < 0) || (slot >= UDP_SLOTS)) {
		return SYSERR;
	}

	mask = disable();

	udptr = &udptab[slot];

	if(udptr->udstate != UDP_USED) {
		restore(mask);
		return SYSERR;
	}

	if(udptr->udcount == 0) {
		udptr->udpid = getpid();
		udptr->udstate = UDP_RECV;
		recvclr();
		msg = recvtime(timeout);
		if((int32)msg == TIMEOUT) {
			udptr->udstate = UDP_USED;
			restore(mask);
			return TIMEOUT;
		}
		if((int32)msg != OK) {
			udptr->udstate = UDP_USED;
			restore(mask);
			return SYSERR;
		}
		udptr->udstate = UDP_USED;
	}

	pkt = udptr->udqueue[udptr->udhead];
	udptr->udhead++;
	if(udptr->udhead >= UDP_QSIZ) {
		udptr->udhead = 0;
	}
	udptr->udcount--;

	udplen = pkt->net_udplen - UDP_HDR_LEN;

	if(udplen < len) {
		len = udplen;
	}

	memcpy(buf, pkt->net_udpdata, len);

	memcpy(ipdata->ipsrc, pkt->net_ipsrc, 16);
	memcpy(ipdata->ipdst, pkt->net_ipdst, 16);
	ipdata->iphl = pkt->net_iphl;

	freebuf((char *)pkt);
	restore(mask);
	return len;
}

/*------------------------------------------------------------------------
 * udp_send  -  Send a UDP packet
 *------------------------------------------------------------------------
 */
int32	udp_send (
	int32	slot,	/* Slot in UDP table	*/
	char	*buf,	/* User data to send	*/
	int32	len	/* Size of data to send	*/
	)
{
	struct	udpentry *udptr;	/* Pointer to UDP table entry	*/
	struct	netpacket *pkt;		/* Pointer to packet buffer	*/
	intmask	mask;			/* Saved interrupt mask		*/

	if((slot < 0) || (slot >= UDP_SLOTS)) {
		return SYSERR;
	}

	mask = disable();

	udptr = &udptab[slot];

	if(udptr->udstate == UDP_FREE) {
		kprintf("udp_send: error1\n");
		restore(mask);
		return SYSERR;
	}

	if(isipunspec(udptr->udremip)) {
		kprintf("udp_send: error2\n");
		restore(mask);
		return SYSERR;
	}

	pkt = (struct netpacket *)getbuf(netbufpool);
	if((int32)pkt == SYSERR) {
		kprintf("udp_send: error3\n");
		restore(mask);
		return SYSERR;
	}

	pkt->net_iface = udptr->udiface;

	pkt->net_ipvtch = 0x60;
	pkt->net_iptclflh = 0;
	pkt->net_ipfll = 0;
	pkt->net_iphl = 255;
	pkt->net_ipnh = IP_UDP;
	pkt->net_iplen = len + UDP_HDR_LEN;
	memcpy(pkt->net_ipsrc, ip_unspec, 16);
	memcpy(pkt->net_ipdst, udptr->udremip, 16);

	pkt->net_udpsport = udptr->udlocport;
	pkt->net_udpdport = udptr->udremport;
	pkt->net_udplen = UDP_HDR_LEN + len;
	pkt->net_udpcksum = 0;

	memcpy(pkt->net_udpdata, buf, len);

	kprintf("udp_send: sending\n");
	ip_enqueue(pkt);

	restore(mask);
	return len;
}

/*------------------------------------------------------------------------
 * udp_cksum  -  Compute checksum of a UDP packet
 *------------------------------------------------------------------------
 */
uint16	udp_cksum (
	struct	netpacket *pkt
	)
{
	uint16	cksum, *ptr16;
	uint32	sum;
	int32	udplen;
	int32	i;
	struct	{
		byte	ipsrc[16];
		byte	ipdst[16];
		uint32	udplen;
		byte	zeros[3];
		byte	ipnh;
	} pseudo;

	udplen = pkt->net_iplen;

	memset(&pseudo, 0, sizeof(pseudo));
	memcpy(pseudo.ipsrc, pkt->net_ipsrc, 16);
	memcpy(pseudo.ipdst, pkt->net_ipdst, 16);
	pseudo.udplen = htonl(pkt->net_iplen);
	pseudo.ipnh = IP_UDP;

	sum = 0;
	ptr16 = (uint16 *)&pseudo;
	for(i = 0; i < 20; i++) {
		sum += htons(*ptr16);
		ptr16++;
	}

	if(udplen % 2) {
		pkt->net_ipdata[udplen] = 0;
		udplen++;
	}

	ptr16 = (uint16 *)pkt->net_ipdata;
	for(i = 0; i < (udplen/2); i++) {
		sum += htons(*ptr16);
		ptr16++;
	}

	cksum = (sum&0xffff) + ((sum>>16)&0xffff);

	return (~cksum);
}

/*------------------------------------------------------------------------
 * udp_hton  -  Convert UDP header fields from network to host order
 *------------------------------------------------------------------------
 */
void	udp_hton (
	struct	netpacket *pkt
	)
{
	pkt->net_udpsport = htons(pkt->net_udpsport);
	pkt->net_udpdport = htons(pkt->net_udpdport);
	pkt->net_udplen = htons(pkt->net_udplen);
}

/*------------------------------------------------------------------------
 * udp_ntoh  -  Convert UDP header fields from network to host order
 *------------------------------------------------------------------------
 */
void	udp_ntoh (
	struct	netpacket *pkt
	)
{
	pkt->net_udpsport = ntohs(pkt->net_udpsport);
	pkt->net_udpdport = ntohs(pkt->net_udpdport);
	pkt->net_udplen = ntohs(pkt->net_udplen);
}
