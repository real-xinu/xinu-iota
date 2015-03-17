/* ip.c - ip_in, ip_recv, ip_hton, ip_ntoh */

#include <xinu.h>

/* IP output queue */
struct	iqentry ipoqueue;

/* IP link local prefix */
byte	ip_llprefix[] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0,
			 0, 0, 0, 0, 0, 0, 0, 0};

/* IP solicited node multicast prefix */
byte	ip_solmc[] = {0xff, 0x02, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 1, 0xff, 0, 0, 0};
/* IP loopback address */
byte	ip_loopback[] = {0, 0, 0, 0, 0, 0, 0, 0,
			 0, 0, 0, 0, 0, 0, 0, 1};

/* IP unspecified address */
byte	ip_unspec[] = {0, 0, 0, 0, 0, 0, 0, 0,
		       0, 0, 0, 0, 0, 0, 0, 0};

/*------------------------------------------------------------------------
 * ip_init  -  Initialize IP data structures
 *------------------------------------------------------------------------
 */
int32	ip_init (void) {

	intmask	mask;	/* Saved interrupt mask	*/

	/* Ensure only one process accesses IP data */

	mask = disable();

	/* Initialize IP output queue head and tail */

	ipoqueue.iqhead = ipoqueue.iqtail = 0;

	/* Create a semaphore for queue management */

	ipoqueue.iqsem = semcreate(IP_OQSIZ);
	if(ipoqueue.iqsem == SYSERR) {
		restore(mask);
		return SYSERR;
	}

	/* Restore interrupts */

	restore(mask);

	/* Create an IP output process */

	resume(create(ipout, NETSTK, NETPRIO, "IP output", 0, NULL));

	return OK;
}

/*------------------------------------------------------------------------
 * ip_in  -  Handle an incoming IP packet
 *------------------------------------------------------------------------
 */
void	ip_in (
	struct	netpacket *pkt
	)
{
	struct	ifentry *ifptr; /* Pointer to interface	*/
	int32	i;		/* For loop index	*/

	/* Check the incoming interface */

	if((pkt->net_iface < 0) || (pkt->net_iface >= NIFACES)) {
		freebuf((char *)pkt);
		return;
	}

	ifptr = &if_tab[pkt->net_iface];

	/* Check IP version */

	if((pkt->net_ipvtch & 0xf0) != 0x60) {
		freebuf((char *)pkt);
		return;
	}

	/* Convert IP fields to host byte order */

	ip_ntoh(pkt);

	/* Match destination against own IP addresses */

	for(i = 0; i < ifptr->if_nipucast; i++) {
		if(memcmp(pkt->net_ipdst, ifptr->if_ipucast[i].ipaddr,
							16) == 0) {
			kprintf("ip_in: iface %d match %d\n", pkt->net_iface, i);
			ip_recv(pkt);
			return;
		}
	}

	for(i = 0; i < ifptr->if_nipmcast; i++) {
		if(!memcmp(pkt->net_ipdst, ifptr->if_ipmcast[i].ipaddr, 16)) {
			kprintf("ip_in: iface %d match %d, mcast\n", pkt->net_iface, i);
			ip_recv(pkt);
			return;
		}
	}
	kprintf("ip_in: no match\n");

	/* Cannot forward link local packets */

	if(isipllu(pkt->net_ipdst)) {
		freebuf((char *)pkt);
		return;
	}

	/* Destination cannot be loopback */

	if(isiplb(pkt->net_ipdst)) {
		freebuf((char *)pkt);
		return;
	}
}

/*------------------------------------------------------------------------
 * ip_recv  -  Handle packet destined for this node
 *------------------------------------------------------------------------
 */
void	ip_recv (
	struct	netpacket *pkt
	)
{
	byte	nxthdr;		/* Next header value	*/
	struct	netpacket *newpkt;/* Decapsulated pkt	*/
	byte	*currptr;	/* Current pointer	*/

	nxthdr = pkt->net_ipnh;
	currptr = pkt->net_ipdata;

	if(nxthdr == IP_EXT_HBH) {
		if(ip_prochbh(pkt->net_ipdata) == SYSERR) {
			freebuf((char *)pkt);
			return;
		}
		currptr = currptr + (*(currptr+1))*8 + 8;
	}

	while(TRUE) {

		switch(nxthdr) {

		 case IP_EXT_DST:

		 case IP_EXT_RH:

		 case IP_EXT_FRG:
		 	freebuf((char *)pkt);
			return;

		 case IP_IPV6:
		 	newpkt = (struct netpacket *)getbuf(netbufpool);
			if((int32)newpkt == SYSERR) {
				freebuf((char *)pkt);
				return;
			}
			memcpy((char *)&newpkt->net_ipvtch,
			       (char *)currptr,
			        pkt->net_iplen - (currptr-pkt->net_ipdata));
			freebuf((char *)pkt);
			ip_in(newpkt);
			return;

		 case IP_ICMPV6:
		 	pkt->net_iplen = pkt->net_iplen - (currptr-pkt->net_ipdata);
			memcpy(pkt->net_ipdata, currptr, pkt->net_iplen);
			kprintf("ip_recv: calling icmp_in\n");
			icmp_in(pkt);
			return;

		 case IP_UDP:
		 	pkt->net_iplen = pkt->net_iplen - (currptr-pkt->net_ipdata);
			memcpy(pkt->net_ipdata, currptr, pkt->net_iplen);
			//udp_in(pkt);
			return;

		 case IP_TCP:
		 	pkt->net_iplen = pkt->net_iplen - (currptr-pkt->net_ipdata);
			memcpy(pkt->net_ipdata, currptr, pkt->net_iplen);
			tcp_in(pkt);
			return;

		 default:
			freebuf((char *)pkt);
			return;
		}
	}
}

/*------------------------------------------------------------------------
 * ip_send  -  Send an IP packet
 *------------------------------------------------------------------------
 */
int32	ip_send (
	struct	netpacket *pkt	/* Pointer to packet buffer	*/
	)
{
	struct	ifentry *ifptr;	/* Pointer to interface	*/
	uint16	cksum;		/* Checksum		*/
	int32	iface;		/* Interface index	*/
	intmask	mask;		/* Saved interrupt mask	*/
	int32	retval;		/* Return value		*/
	int32	i;		/* For loop index	*/

	if(pkt == NULL) {
		return SYSERR;
	}

	mask = disable();

	iface = pkt->net_iface;
	if((iface < 0) || (iface >= NIFACES)) {
		kprintf("ip_send: invalid interface %d\n", iface);
		freebuf((char *)pkt);
		return SYSERR;
	}

	ifptr = &if_tab[iface];
	if(ifptr->if_state == IF_DOWN) {
		kprintf("ip_send: interface %d down\n", iface);
		freebuf((char *)pkt);
		return SYSERR;
	}

	if(isipllu(pkt->net_ipdst) || isipmc(pkt->net_ipdst)) {
		if(isipunspec(pkt->net_ipsrc)) {
			memcpy(pkt->net_ipsrc, ifptr->if_ipucast[0].ipaddr, 16);
		}
	}

	switch(pkt->net_ipnh) {
	
	 case IP_ICMPV6:
		 pkt->net_iccksum = 0;
		 cksum = icmp_cksum(pkt);
		 pkt->net_iccksum = htons(cksum);
		 break;
	}

	if(ifptr->if_type == IF_TYPE_RADIO) {
		retval = ip_send_rad(pkt);
		restore(mask);
		return retval;
	}
	else if(ifptr->if_type == IF_TYPE_ETH) {
		kprintf("ip_send: calling ip_send_eth\n");
		retval = ip_send_eth(pkt);
		restore(mask);
		return retval;
	}
	
	freebuf((char *)pkt);
	restore(mask);
	return SYSERR;
}

/*------------------------------------------------------------------------
 * ip_send_rad  -  Send an IP packet over the RADIO interface
 *------------------------------------------------------------------------
 */
int32	ip_send_rad (
	struct	netpacket *pkt	/* Pointer to packet buffer	*/
	)
{
	struct	ifentry *ifptr;	/* Pointer to interface	*/
	int32	iface;		/* Interface index	*/
	intmask	mask;		/* Saved interrupt mask	*/
	int32	i;		/* For loop index	*/

	iface = pkt->net_iface;
	if((iface < 0) || (iface >= NIFACES)) {
		freebuf((char *)pkt);
	}

	mask = disable();

	ifptr = &if_tab[iface];
	if(ifptr->if_type != IF_TYPE_RADIO) {
		restore(mask);
		return SYSERR;
	}

	if(isipmc(pkt->net_ipdst)) {
		if(!memcmp(pkt->net_ipsrc, ip_unspec, 16)) {
			memcpy(pkt->net_ipsrc, ifptr->if_ipucast[0].ipaddr, 16);
		}
		memset(pkt->net_raddstaddr, 0xff, 8);
		memcpy(pkt->net_radsrcaddr, ifptr->if_hwucast, 8);

		ip_hton(pkt);
		write(RADIO0, (char *)pkt, 24+40+ntohs(pkt->net_iplen));
		freebuf((char *)pkt);
		restore(mask);
		return OK;
	}

	if(isipllu(pkt->net_ipdst)) {
		if(!memcmp(pkt->net_ipsrc, ip_unspec, 16)) {
			memcpy(pkt->net_ipsrc, ifptr->if_ipucast[0].ipaddr, 16);
		}
		memcpy(pkt->net_raddstaddr, &pkt->net_ipdst[8], 8);
		memcpy(pkt->net_radsrcaddr, ifptr->if_hwucast, 8);

		ip_hton(pkt);

		write(RADIO0, (char *)pkt, 24+40+ntohs(pkt->net_iplen));
		freebuf((char *)pkt);
		restore(mask);
		return OK;
	}

	kprintf("Looking in nbr cache\n");
	for(i = 0; i < ND_NC_SLOTS; i++) {
		if(ifptr->if_ncache[i].state == ND_NCE_FREE) {
			continue;
		}
		if(!memcmp(ifptr->if_ncache[i].ipaddr, pkt->net_ipdst, 16)) {
			break;
		}
	}
	if(i < ND_NC_SLOTS) {
		kprintf("Foudn in nbr cache\n");
		memcpy(pkt->net_raddstaddr, ifptr->if_ncache[i].hwucast, 8);
		memcpy(pkt->net_radsrcaddr, ifptr->if_hwucast, 8);

		ip_hton(pkt);

		write(RADIO0, (char *)pkt, 24+40+htons(pkt->net_iplen));
		freebuf((char *)pkt);
		restore(mask);
		return OK;
	}

	kprintf("Not found in nbr cache\n");
	freebuf((char *)pkt);

	restore(mask);
	return SYSERR;
}

/*------------------------------------------------------------------------
 * ip_send_eth  -  Send an IP packet over Ethernet interface
 *------------------------------------------------------------------------
 */
int32	ip_send_eth (
	struct	netpacket *pkt
	)
{
	struct	ifentry *ifptr;	/* Pointer to interface	*/
	int32	iface;		/* Index interface	*/
	intmask	mask;		/* Saved interrupt mask	*/
	int32	pktlen;		/* Length of packet	*/
	int32	retval;		/* Return value		*/
	int32	i;		/* For loop index	*/

	if(pkt == NULL) {
		return SYSERR;
	}

	mask = disable();

	iface = pkt->net_iface;
	if((iface < 0) || (iface >= NIFACES)) {
		kprintf("ip_send_eth: invalide interface %d\n", iface);
		restore(mask);
		freebuf((char *)pkt);
		return SYSERR;
	}

	ifptr = &if_tab[iface];
	if(ifptr->if_type != IF_TYPE_ETH) {
		kprintf("ip_send_eth: interface %d, not ethernet\n", iface);
		freebuf((char*)pkt);
		restore(mask);
		return SYSERR;
	}

	if(isipmc(pkt->net_ipdst)) {
		memcpy(pkt->net_ipsrc, ifptr->if_ipucast[0].ipaddr, 16);
		pkt->net_ethdst[0] = 0x33;
		pkt->net_ethdst[1] = 0x33;
		memcpy(&pkt->net_ethdst[2], &pkt->net_ipdst[13], 4);
		memcpy(pkt->net_ethsrc, ifptr->if_hwucast, 6);
		pkt->net_ethtype = htons(ETH_IPV6);

		pktlen = 14 + 40 + pkt->net_iplen;
		ip_hton(pkt);

		kprintf("OUT: "); pdump(pkt);
		pkt->net_ethpad[0] = 0;
		write(ifptr->if_dev, (char *)pkt, pktlen);
		freebuf((char *)pkt);
		restore(mask);
		return OK;
	}

	kprintf("ip_send_eth: calling nd_resolve\n");
	retval = nd_resolve_eth(pkt->net_iface, pkt->net_ipdst, pkt->net_ethdst);
	if((retval == SYSERR) || (retval == TIMEOUT)) {
		restore(mask);
		return SYSERR;
	}

	memcpy(pkt->net_ethsrc, ifptr->if_hwucast, 6);
	pkt->net_ethtype = htons(ETH_IPV6);

	pktlen = 14 + 40 + pkt->net_iplen;
	ip_hton(pkt);

	kprintf("OUT: "); pdump(pkt);
	pkt->net_ethpad[0] = 0;
	write(ifptr->if_dev, (char *)pkt, pktlen);
	freebuf((char *)pkt);
	restore(mask);

	return OK;
}

/*------------------------------------------------------------------------
 * ip_prochbh  -  Process the Hop-By-Hop extension header
 *------------------------------------------------------------------------
 */
int32	ip_prochbh (
	byte	*hdr	/* Start of header	*/
	)
{
	byte	totallen;	/* Total header length	*/
	byte	opttype;	/* Option type		*/
	byte	optlen;		/* Option length	*/
	byte	*optptr;	/* Option pointer	*/
	byte	*hdrend;	/* One past header	*/

	totallen = (*(hdr + 1))*8 + 8;
	hdrend = hdr + totallen;

	while(optptr < hdrend) {

		opttype = *optptr;
		optlen = *(optptr + 1);

		switch (opttype) {

		 case IP_OPT_PAD1:
		 	optptr++;
			break;

		 case IP_OPT_PADN:
		 	optptr = optptr + optlen;
			break;

		 default:
		 	if((opttype & 0xc0) != 0x00) {
				return SYSERR;
			}
			optptr = optptr + optlen;
			break;
		}
	}

	return OK;
}

/*------------------------------------------------------------------------
 * ip_enqueue  -  Enqueue an IP datagram for transmission
 *------------------------------------------------------------------------
 */
int32	ip_enqueue (
	struct	netpacket *pkt
	)
{
	intmask	mask;

	mask = disable();

	if(semcount(ipoqueue.iqsem) >= IP_OQSIZ) {
		restore(mask);
		return SYSERR;
	}

	ipoqueue.iqbuf[ipoqueue.iqtail] = pkt;
	ipoqueue.iqtail += 1;
	if(ipoqueue.iqtail >= IP_OQSIZ) {
		ipoqueue.iqtail = 0;
	}

	signal(ipoqueue.iqsem);

	restore(mask);
	return OK;
}

/*------------------------------------------------------------------------
 * ipout  -  Process to transmit IP datagrams
 *------------------------------------------------------------------------
 */
process	ipout(void) {

	struct	netpacket *pkt;
	intmask	mask;

	while(TRUE) {

		wait(ipoqueue.iqsem);

		mask = disable();

		pkt = ipoqueue.iqbuf[ipoqueue.iqhead];
		ipoqueue.iqhead += 1;
		if(ipoqueue.iqhead >= IP_OQSIZ) {
			ipoqueue.iqhead = 0;
		}

		restore(mask);

		ip_send(pkt);
	}

	return OK;
}

/*------------------------------------------------------------------------
 * ip_hton  -  Convert Ip fields in network byte order
 *------------------------------------------------------------------------
 */
void	ip_hton (
	struct	netpacket *pkt
	)
{
	pkt->net_ipfll = htons(pkt->net_ipfll);
	pkt->net_iplen = htons(pkt->net_iplen);
}

/*------------------------------------------------------------------------
 * ip_ntoh  -  Convert IP fields in host byte order
 *------------------------------------------------------------------------
 */
void	ip_ntoh (
	struct	netpacket *pkt
	)
{
	pkt->net_ipfll = ntohs(pkt->net_ipfll);
	pkt->net_iplen = ntohs(pkt->net_iplen);
}

/*------------------------------------------------------------------------
 * ip_print  -  Print IPv6 address in compressed form
 *------------------------------------------------------------------------
 */
void	ip_print (
	byte	*ip
	)
{
	int32	i;
	uint16	*ptr16;

	if(ip == NULL) {
		return;
	}

	ptr16 = (uint16 *)ip;

	while((i < 8) && ((*ptr16)!=0)) {
		kprintf("%X", htons(*ptr16));
		i++;
		ptr16++;
		if(i <= 7) {
			kprintf(":");
		}
	}

	if((i < 8) && ((*ptr16)==0)) {
		kprintf(":");
		while((i < 8) && ((*ptr16)==0)) {
			i++;
			ptr16++;
		}
	}

	while(i < 8) {
		kprintf("%X", htons(*ptr16));
		i++;
		ptr16++;
		if(i <= 7) {
			kprintf(":");
		}
	}
}
