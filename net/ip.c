/* ip.c - ip_in */

#include <xinu.h>

/* IP Link-local prefix */
byte	ip_llprefix[] = { 0xfe, 0x80, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0 };

/* Solicited-node Multicast prefix */
byte	ip_nd_snmcprefix[] = { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 1, 0xff, 0, 0, 0};

/* All-nodes multicast address */
byte	ip_allnodesmc[] = { 0xff, 0x01, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 1};

/* Unspecified IP address */
byte	ip_unspec[16] = {0};

/* IP forwarding table */
struct	ip_fwentry ip_fwtab[IPFW_TABSIZE];

/*------------------------------------------------------------------------
 * ip_in  -  Handle incoming IPv6 packets
 *------------------------------------------------------------------------
 */
void	ip_in (
		struct	netpacket *pkt	/* Packet buffer pointer	*/
	      )
{
	struct	netiface *ifptr;	/* Network interface pointer	*/
	intmask	mask;			/* Interrupt mask		*/
	int32	i;			/* Index variable		*/

	/* Verify that the version is 6 */
	if((pkt->net_ipvtch & 0xf0) != 0x60) {
		kprintf("IP version incorrect\n");
		return;
	}

	/* Check the interface on which the packet has arrived */
	if(pkt->net_iface < 0 || pkt->net_iface >= NIFACES) {
		kprintf("Invalid interface number\n");
		return;
	}

	ifptr = &iftab[pkt->net_iface];

	/* Convert fields to host-byte order */
	ip_ntoh(pkt);

	//kprintf("Incoming IPv6 packet for "); ip_printaddr(pkt->net_ipdst); kprintf("\n");

	mask = disable();

	/* Match the destination address against our addresses */
	if(!isipmc(pkt->net_ipdst)) {

		for(i = 0; i < ifptr->if_nipucast; i++) {
			if(!memcmp(pkt->net_ipdst, 
					ifptr->if_ipucast[i].ipaddr, 16)) {
				break;
			}
		}
		if(i >= ifptr->if_nipucast) {
			//forward the packet
			restore(mask);
			return;
		}
	}
	else {

		for(i = 0; i < ifptr->if_nipmcast; i++) {
			if(!memcmp(pkt->net_ipdst, 
					ifptr->if_ipmcast[i].ipaddr, 16)) {
				break;
			}
		}
		if(i >= ifptr->if_nipmcast) {
			restore(mask);
			return;
		}
	}

	/* If we are here, that means it is either a local packet or	*/
	/* a source routed packet. Let's process the extensions		*/

	ip_in_ext(pkt);

	restore(mask);
}

/*------------------------------------------------------------------------
 * ip_in_ext  -  Process the extension headers of the packet
 *------------------------------------------------------------------------
 */
void	ip_in_ext (
		struct	netpacket *pkt	/* Packet pointer	*/
		)
{
	struct	ip_ext_hdr *exptr;	/* Extension header pointer	*/
	byte	tmpaddr[16];		/* Temporary address buffer	*/
	byte	nh;			/* Next header value		*/
	bool8	first;			/* Flag indicates first header	*/
	int32	l4len;			/* Layer 4 data length		*/
	int32	i;			/* Index variable		*/

	/* Get the next header value */
	nh = pkt->net_ipnh;

	/* Get a pointer to extension header */
	exptr = (struct ip_ext_hdr *)pkt->net_ipdata;

	/* This is the first header */
	first = TRUE;

	switch(nh) {

	/* Hop-by-hop header */
	case IP_EXT_HBH:

		first = FALSE;

		/* If hop-by-hop is not the first header, discard packet*/
		if(!first) {
			//freebuf_np(pkt);
			return;
		}

		//Process the options

		nh = exptr->ipext_nh;
		exptr = (struct ip_ext_hdr *)((char *)exptr + 8 + exptr->ipext_len * 8);
		break;
	
	/* Routing Header */
	case IP_EXT_RH:

		first = FALSE;

		if(exptr->ipext_rhrt == 0) { /* Routing type = 0 */

			if(exptr->ipext_rhsegs == 0) {
				nh = exptr->ipext_nh;
				exptr = (struct ip_ext_hdr *)((char *)exptr + 8 + exptr->ipext_len * 8);
				break;
			}

			if(exptr->ipext_len & 0x1) {
				//Send ICMP Parameter problem
				return;
			}

			if(exptr->ipext_rhsegs > (exptr->ipext_len / 2)) {
				//Send ICMP Parameter problem
				return;
			}

			exptr->ipext_rhsegs--;

			i = (exptr->ipext_len / 2) - exptr->ipext_rhsegs;

			if(isipmc(exptr->ipext_rhaddrs[i])) {
				return;
			}

			memcpy(tmpaddr, pkt->net_ipdst, 16);
			memcpy(pkt->net_ipdst, exptr->ipext_rhaddrs[i], 16);
			memcpy(exptr->ipext_rhaddrs[i], tmpaddr, 16);

			if(pkt->net_iphl <= 1) {
				//Send ICMP Time Exceeded
				return;
			}

			pkt->net_iphl--;

			//Forward the packet
			return;
		}
		else { /* Unrecognized routing type */
			return;
		}
		nh = exptr->ipext_nh;
		exptr = (struct ip_ext_hdr *)((char *)exptr + 8 + exptr->ipext_len * 8);
		break;

	//IP_UDP:
	//IP_TCP:
	case IP_ICMP:
		/* Process ICMP packet */
		if((char *)exptr != (char *)pkt->net_ipdata) {
			l4len = pkt->net_iplen - ((char *)exptr - (char *)pkt->net_ipdata);
			memcpy(pkt->net_ipdata, exptr, l4len);
			pkt->net_iplen = l4len;
		}
		icmp_in(pkt);
		return;

	default:
		kprintf("Unknown IP next header: %02x. Discarding packet\n", nh);
	}
}

/*------------------------------------------------------------------------
 * ip_route  -  Route an IP packet
 *------------------------------------------------------------------------
 */
int32	ip_route (
		struct	netpacket *pkt,	/* Packet buffer pointer	*/
		byte	nxthop[]	/* Next hop buffer address	*/
		)
{
	struct	ip_fwentry *ipfptr;	/* IP forwarding table entry ptr*/
	intmask	mask;			/* Interrupt mask		*/
	int32	found;			/* Flag indicating found entry	*/
	int32	maxlen;			/*				*/
	int32	i;			/* Index variable		*/

	mask = disable();

	/* If destination is multicast... */
	if(isipmc(pkt->net_ipdst)) {

		/* Packet should already have an interface */
		if(pkt->net_iface < 0 || pkt->net_iface >= NIFACES) {
			restore(mask);
			return SYSERR;
		}

		//Must be changed. Right now uses link-local source
		memcpy(pkt->net_ipsrc, iftab[pkt->net_iface].if_ipucast[0].ipaddr, 16);
		restore(mask);
		return OK;
	}

	/* If destination is link-local... */
	if(isipllu(pkt->net_ipdst)) {

		/* Packet must already have an interface */
		if(pkt->net_iface < 0 || pkt->net_iface >= NIFACES) {
			restore(mask);
			return SYSERR;
		}

		/* IP source is the interface's link-local address */
		memcpy(pkt->net_ipsrc, iftab[pkt->net_iface].if_ipucast[0].ipaddr, 16);

		/* Next hop is the destination itself */
		memcpy(nxthop, pkt->net_ipdst, 16);

		restore(mask);
		return OK;
	}

	/* If we are here, we must go through the forwarding table */

	found = -1;
	maxlen = 0;

	/* Look for the longest matching prefix */

	for(i = 0; i < IPFW_TABSIZE; i++) {
		if(ip_fwtab[i].ipfw_state == IPFW_STATE_FREE) {
			continue;
		}

		ipfptr = &ip_fwtab[i];
		if(!memcmp(pkt->net_ipdst, ipfptr->ipfw_prefix.ipaddr,
					ipfptr->ipfw_prefix.prefixlen)) {
			if(ipfptr->ipfw_prefix.prefixlen > maxlen) {
				found = i;
			}
		}
	}

	/* If no prefix matches, return error */
	if(found == -1) {
		restore(mask);
		return SYSERR;
	}

	ipfptr = &ip_fwtab[found];

	pkt->net_iface = ipfptr->ipfw_iface;

	/* Check if prefix is on-link */
	if(ipfptr->ipfw_onlink) { /* Destination is next hop */
		memcpy(nxthop, pkt->net_ipdst, 16);
	}
	else { /* Get the next hop from table entry */
		memcpy(nxthop, ipfptr->ipfw_nxthop, 16);
	}

	return OK;
}

/*------------------------------------------------------------------------
 * ip_send  -  Send an IP packet
 *------------------------------------------------------------------------
 */
int32	ip_send (
		struct	netpacket *pkt	/* Packet buffer pointer	*/
		)
{
	struct	nd_ncentry *ncptr;	/* Neighbor cache entry		*/
	struct	netiface *ifptr;	/* Network interface pointer	*/
	struct	netpacket_e *epkt;	/* Ethernet packet pointer	*/
	byte	nxthop[16];		/* Next hop address buffer	*/
	int32	ncindex;		/* Index into neighbor cache	*/
	uint16	cksum;			/* Checksum			*/
	int32	iplen;			/* IP payload length		*/
	int32	retval;			/* Return value variable	*/
	intmask	mask;			/* Interrupt mask		*/

	/* Route the packet */
	retval = ip_route(pkt, nxthop);

	//kprintf("ip_send: ip_route returns %d\n", retval);

	/* If we cannot route the packet, discard it and return */
	if(retval == SYSERR) {
		freebuf((char *)pkt);
		return SYSERR;
	}

	/* Process the packet according to its type */
	switch(pkt->net_ipnh) {

	case IP_ICMP:
		pkt->net_iccksum = 0;
		cksum = icmp_cksum(pkt);
		pkt->net_iccksum = htons(cksum);
		break;

	}

	/* Convert IP fields in network-byte order */
	ip_hton(pkt);

	mask = disable();

	ifptr = &iftab[pkt->net_iface];

	ncptr = NULL;

	/* If destination is not multicast... */
	if(!isipmc(pkt->net_ipdst)) {

		/* Look for the next hop in the neighbor cache */
		ncindex = nd_ncfind(nxthop);

		/* If no entry found, resolve the next hop */
		if(ncindex == SYSERR) {
			nd_resolve(nxthop, pkt->net_iface, pkt);
			restore(mask);
			return OK;
		}

		ncptr = &nd_ncache[ncindex];

		/* If entry is found, look at the state */
		switch(ncptr->nc_rstate) {

		case NC_RSTATE_INC:
			/* Next hop is being resolved, enqueue packet */
			if(ncptr->nc_pqcount == NC_PKTQ_SIZE) {
				freebuf((char *)ncptr->nc_pktq[ncptr->nc_pqtail]);
				ncptr->nc_pktq[ncptr->nc_pqtail] = pkt;
			}
			else {
				ncptr->nc_pktq[ncptr->nc_pqtail++] = pkt;
				if(ncptr->nc_pqtail >= NC_PKTQ_SIZE) {
					ncptr->nc_pqtail = 0;
				}
				ncptr->nc_pqcount++;
			}
			restore(mask);
			return OK;

		case NC_RSTATE_STL:
			/* Change the state to DELAY */
			ncptr->nc_rstate = NC_RSTATE_DLY;
			ncptr->nc_texpire = ND_DELAY_FIRST_PROBE_TIME;
			break;
		}
	}

	/* Check the type of the interface */
	if(ifptr->if_type == IF_TYPE_ETH) { /* Ethernet interface */

		/* Allocate an ethernet packet buffer */
		epkt = (struct netpacket_e *)getbuf(netbufpool);
		if((int32)epkt == SYSERR) {
			restore(mask);
			return SYSERR;
		}

		/* Copy the ethernet source from interface */
		memcpy(epkt->net_ethsrc, ifptr->if_hwucast, ifptr->if_halen);

		/* Set the ethernet dest according to whether the 	*/
		/* ip dst is multicast or unicast			*/
		if(!isipmc(pkt->net_ipdst)) {
			memcpy(epkt->net_ethdst, ncptr->nc_hwaddr, ifptr->if_halen);
		}
		else {
			epkt->net_ethdst[0] = 0x33;
			epkt->net_ethdst[1] = 0x33;
			memcpy(&epkt->net_ethdst[2], &pkt->net_ipdst[12], 4);
		}
		epkt->net_ethtype = htons(ETH_IPV6);

		iplen = 40 + ntohs(pkt->net_iplen);

		/* Copy the IP packet into the ethernet packet */
		memcpy(epkt->net_ethdata, (char *)pkt, iplen);

		//kprintf("Sending IPv6 packet\n");
		//for(i = 0; i < 14+iplen; i++) {
		//	kprintf("%02x ", *((byte *)epkt + i));
		//}
		//kprintf("\n");

		/* Send the packet over the device */
		write(ETHER0, (char *)epkt, 14 + iplen);

		freebuf((char *)epkt);
		freebuf((char *)pkt);
		restore(mask);
		return OK;
	}
	else {
		restore(mask);
		return OK;
	}
}

/*------------------------------------------------------------------------
 * ip_hton  -  Convert IP header fields in network byte order
 *------------------------------------------------------------------------
 */
void	ip_hton (
		struct	netpacket *pkt
		)
{
	pkt->net_iplen = ntohs(pkt->net_iplen);
}

/*------------------------------------------------------------------------
 * ip_ntoh  -  Convert IP header fields in host byte order
 *------------------------------------------------------------------------
 */
void	ip_ntoh (
		struct	netpacket *pkt
		)
{
	pkt->net_iplen = ntohs(pkt->net_iplen);
}

/*------------------------------------------------------------------------
 * ip_printaddr  -  Print an IP address in a readable format
 *------------------------------------------------------------------------
 */
void	ip_printaddr (
		byte	*ipaddr
		)
{
	int32	i;
	uint16	*ptr16;

	ptr16 = (uint16 *)ipaddr;

	for(i = 0; i < 7; i++) {
		kprintf("%04X:", htons(*ptr16));
		ptr16++;
	}
	kprintf("%04X", htons(*ptr16));
}
