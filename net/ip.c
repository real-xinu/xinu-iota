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

/* ALL RPL nodes multicast address */
byte	ip_allrplnodesmc[] = { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0x1a};

/* Unspecified IP address */
byte	ip_unspec[16] = {0};

/* IP forwarding table */
struct	ip_fwentry ip_fwtab[IPFW_TABSIZE];
//#define DEBUG_IP 1
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

	#ifdef DEBUG_IP
	kprintf("ip_in: incoming IP packet %d\n", pkt->net_iplen);
	#endif

	/* Verify that the version is 6 */
	if((pkt->net_ipvtch & 0xf0) != 0x60) {
		kprintf("IP version incorrect\n");
		return;
	}

	/* Check the interface on which the packet has arrived */
	if(pkt->net_iface < 0 || pkt->net_iface >= NIFACES) {
		kprintf("Invalid interface number %d\n", pkt->net_iface);
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
			//TODO forward the packet
			#ifdef DEBUG_IP
			kprintf("ip_in: must forward\n");
			#endif
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
			#ifdef DEBUG_IP
			kprintf("ip_in: multicast address not found\n");
			#endif
			restore(mask);
			return;
		}
	}

	/* If we are here, that means it is either a local packet or	*/
	/* a source routed packet. Let's process the extensions		*/

	#ifdef DEBUG_IP
	kprintf("ip_in: calling ip_in_ext\n");
	#endif

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
	struct	netpacket *inpkt;	/* Inner IP packet		*/
	struct	netpacket *fwpkt;	/* Packet to be forwarded	*/
	struct	ip_ext_hdr *exptr;	/* Extension header pointer	*/
	struct	netiface *ifptr;	/* Interface pointer		*/
	byte	tmpaddr[16];		/* Temporary address buffer	*/
	byte	nh;			/* Next header value		*/
	bool8	first;			/* Flag indicates first header	*/
	int32	l4len;			/* Layer 4 data length		*/
	int32	i;			/* Index variable		*/

	/* Get the interface pointer */
	ifptr = &iftab[pkt->net_iface];

	/* Get the next header value */
	nh = pkt->net_ipnh;

	/* Get a pointer to extension header */
	exptr = (struct ip_ext_hdr *)pkt->net_ipdata;

	/* This is the first header */
	first = TRUE;

	while(TRUE) {

		switch(nh) {

		/* Hop-by-hop header */
		case IP_EXT_HBH:

			/* If hop-by-hop is not the first header, discard packet*/
			if(!first) {
				//freebuf_np(pkt);
				return;
			}

			first = FALSE;

			#ifdef DEBUG_IP
			kprintf("ip_in_ext: hbh, next %02x\n", exptr->ipext_nh);
			#endif
			//Process the options

			nh = exptr->ipext_nh;
			exptr = (struct ip_ext_hdr *)((char *)exptr + 8 + exptr->ipext_len * 8);
			break;
		
		/* Routing Header */
		case IP_EXT_RH:

			first = FALSE;

			#ifdef DEBUG_IP
			kprintf("ip_in_ext: routing hdr..\n");
			#endif
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

				#ifdef DEBUG_IP
				kprintf("Copying %d ", i-1); ip_printaddr(exptr->ipext_rhaddrs[i-1]);
				kprintf(" in destination\n");
				#endif
				memcpy(tmpaddr, pkt->net_ipdst, 16);
				memcpy(pkt->net_ipdst, exptr->ipext_rhaddrs[i-1], 16);
				memcpy(exptr->ipext_rhaddrs[i-1], tmpaddr, 16);

				if(pkt->net_iphl <= 1) {
					//Send ICMP Time Exceeded
					return;
				}

				pkt->net_iphl--;

				#ifdef DEBUG_IP
				kprintf("ip_in_ext: rt forward packet..%d\n", pkt->net_iplen);
				#endif
				fwpkt = (struct netpacket *)getbuf(netbufpool);
				memcpy(fwpkt, pkt, 40 + pkt->net_iplen);
				ip_send(fwpkt);
				//Forward the packet
				return;
			}
			else { /* Unrecognized routing type */
				return;
			}
			nh = exptr->ipext_nh;
			exptr = (struct ip_ext_hdr *)((char *)exptr + 8 + exptr->ipext_len * 8);
			break;

		case IP_IP:
			inpkt = (struct netpacket *)exptr;
			#ifdef DEBUG_IP
			kprintf("ip_in_ext: IP in IP encapsulation. Dst: ");
			ip_printaddr(inpkt->net_ipdst);
			kprintf("\n");
			#endif
			for(i = 0; i < ifptr->if_nipucast; i++) {
				if(!memcmp(inpkt->net_ipdst, ifptr->if_ipucast[i].ipaddr, 16)) {
					break;
				}
			}
			if(i >= ifptr->if_nipucast) {
				//Forward the packet
				#ifdef DEBUG_IP
				kprintf("ip_in_ext: must forward\n");
				#endif
				fwpkt = (struct netpacket *)getbuf(netbufpool);
				memcpy(fwpkt, inpkt, 40 + ntohs(inpkt->net_iplen));
				ip_ntoh(fwpkt);
				ip_send(fwpkt);
				return;
			}

			inpkt->net_iface = pkt->net_iface;
			ip_ntoh(inpkt);
			pkt = inpkt;
			nh = pkt->net_ipnh;
			exptr = (struct ip_ext_hdr *)pkt->net_ipdata;
			break;

		//IP_UDP:
		case IP_ICMP:
			#ifdef DEBUG_IP
			kprintf("ip_in_ext: icmp. Dst: ");
			ip_printaddr(pkt->net_ipdst);
			kprintf("\n");
			#endif
			/* Process ICMP packet */
			if((char *)exptr != (char *)pkt->net_ipdata) {
				l4len = pkt->net_iplen - ((char *)exptr - (char *)pkt->net_ipdata);
				#ifdef DEBUG_IP
				kprintf("ip_in_ext: icmp. removing extensions. l4len: %d, exptr: %08x, %08x\n", l4len, exptr, pkt->net_ipdata);
				#endif
				memcpy(pkt->net_ipdata, exptr, l4len);
				pkt->net_iplen = l4len;
			}
			#ifdef DEBUG_IP
			kprintf("ip_in_ext: calling icmp_in\n");
			#endif
			icmp_in(pkt);
			return;

		case IP_TCP:
			if(tcpcksum(pkt) != 0) {
				return;
			}
			tcp_ntoh(pkt);
			tcp_in(pkt);
			return;
		case IP_UDP:
		        if (udp_cksum(pkt) != 0){
			      return;
			}
			udp_ntoh(pkt);
			udp_in(pkt);
			return;

		default:
			kprintf("Unknown IP next header: %02x. Discarding packet\n", nh);
			return;
		}
	}
}

/*------------------------------------------------------------------------
 * ip_route  -  Route an IP packet
 *------------------------------------------------------------------------
 */
int32	ip_route (
		byte	ipdst[],	/* IP destination address	*/
		byte	ipsrc[],	/* Ip source to be filled	*/
		byte	nxthop[],	/* Next hop buffer address	*/
		int32	*ifaceptr	/* Interface index pointer	*/
		)
{
	struct	ip_fwentry *ipfptr;	/* IP forwarding table entry ptr*/
	struct	netiface *ifptr;	/* Network interface pointer	*/
	intmask	mask;			/* Interrupt mask		*/
	int32	found;			/* Flag indicating found entry	*/
	int32	maxlen;			/*				*/
	int32	iface;
	int32	i;			/* Index variable		*/

	mask = disable();

	iface = *ifaceptr;

	/* If destination is multicast... */
	if(isipmc(ipdst)) {

		/* Packet should already have an interface */
		if(iface < 0 || iface >= NIFACES) {
			restore(mask);
			return SYSERR;
		}

		//Must be changed. Right now uses link-local source
		memcpy(ipsrc, iftab[iface].if_ipucast[0].ipaddr, 16);
		restore(mask);
		return OK;
	}

	/* If destination is link-local... */
	if(isipllu(ipdst)) {

		/* Packet must already have an interface */
		if(iface < 0 || iface >= NIFACES) {
			restore(mask);
			return SYSERR;
		}

		if(isipunspec(ipsrc)) {
			/* IP source is the interface's link-local address */
			memcpy(ipsrc, iftab[iface].if_ipucast[0].ipaddr, 16);
		}

		if(nxthop) {
			/* Next hop is the destination itself */
			memcpy(nxthop, ipdst, 16);
		}

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

		if(ipfptr->ipfw_prefix.prefixlen == 0) {
			found = i;
		}
		else if(!memcmp(ipdst, ipfptr->ipfw_prefix.ipaddr,
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

	*ifaceptr = ipfptr->ipfw_iface;

	if(nxthop) {
		/* Check if prefix is on-link */
		if(ipfptr->ipfw_onlink) { /* Destination is next hop */
			memcpy(nxthop, ipdst, 16);
		}
		else { /* Get the next hop from table entry */
			memcpy(nxthop, ipfptr->ipfw_nxthop, 16);
		}
	}

	if(isipunspec(ipsrc)) {
		#ifdef DEBUG_IP
		kprintf("ip_route: choosing src\n");
		#endif
		ifptr = &iftab[ipfptr->ipfw_iface];
		for(i = 1; i < ifptr->if_nipucast; i++) {
			if(!memcmp(ipdst, ifptr->if_ipucast[i].ipaddr,
						ifptr->if_ipucast[i].prefixlen)) {
				memcpy(ipsrc, ifptr->if_ipucast[i].ipaddr, 16);
				break;
			}
		}
		if(i >= ifptr->if_nipucast) {
			memcpy(ipsrc, ifptr->if_ipucast[1].ipaddr, 16);
		}
	}

	#ifdef DEBUG_IP
	kprintf("ip_route: src ");
	ip_printaddr(ipsrc);
	kprintf("\n");
	#endif
	restore(mask);
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

	#ifdef DEBUG_IP
	kprintf("ip_send: sending to "); ip_printaddr(pkt->net_ipdst);
	kprintf("\n");
	#endif

	/* Route the packet */
	retval = ip_route(pkt->net_ipdst, pkt->net_ipsrc, nxthop, &pkt->net_iface);

	#ifdef DEBUG_IP
	kprintf("ip_send: ip_route returns %d\n", retval);
	kprintf("ip_send: nxthop: "); ip_printaddr(nxthop);
	kprintf("\n");
	kprintf("ip_send: iface = %d\n", pkt->net_iface);
	#endif

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

	case IP_TCP:
		tcp_hton(pkt);
		pkt->net_tcpcksum = 0;
		cksum = tcpcksum(pkt);
		pkt->net_tcpcksum = htons(cksum);
		break;
	case IP_UDP:
	        udp_hton(pkt);
		pkt->net_udpcksum = 0;
		cksum = udp_cksum(pkt);
		pkt->net_udpcksum = htons(cksum);
		break;
	}

	/* Convert IP fields in network-byte order */
	ip_hton(pkt);

	mask = disable();

	ifptr = &iftab[pkt->net_iface];

	/* If this is to be sent on radio, RPL rules apply */
	if(ifptr->if_type == IF_TYPE_RAD) {
		ip_send_rpl(pkt, nxthop);
		restore(mask);
		return OK;
	}

	ncptr = NULL;

	/* If destination is not multicast... */
	if(ifptr->if_type == IF_TYPE_RAD && isipllu(nxthop)) {
		//resolve L3 to L2 using L3
	}
	else if(!isipmc(pkt->net_ipdst)) {

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

		/*
		int32	i;
		kprintf("Sending IPv6 packet\n");
		for(i = 0; i < 14+iplen; i++) {
			kprintf("%02x ", *((byte *)epkt + i));
		}
		kprintf("\n");
		*/

		/* Send the packet over the device */
		write(ETHER0, (char *)epkt, 14 + iplen);

		freebuf((char *)epkt);
		freebuf((char *)pkt);
		restore(mask);
		return OK;
	}
	else {
		freebuf((char *)pkt);
		restore(mask);
		return OK;
	}
}

/*------------------------------------------------------------------------
 * ip_send_rpl  -  Send IP datagrams using RPL protocol rules
 *------------------------------------------------------------------------
 */
int32	ip_send_rpl (
		struct	netpacket *pkt,	/* Packet buffer	*/
		byte	nxthop[]	/* Next-hop IP address	*/
		)
{
	struct	netpacket_r *rpkt;	/* Radio packet pointer	*/
	struct	netpacket *npkt;	/* IP packet pointer	*/
	struct	ip_ext_hdr *exptr;	/* IP extension header	*/
	struct	netiface *ifptr;	/* Network interface	*/
	intmask	mask;			/* Interrupt mask	*/
	int32	iplen;			/* IP length		*/
	int32	ncindex;		/* Index in nbr. cache	*/

	mask = disable();

	ifptr = &iftab[pkt->net_iface];

	if(ifptr->if_type != IF_TYPE_RAD) {
		restore(mask);
		freebuf((char *)pkt);
		return SYSERR;
	}

	/* Allocate a buffer for radio packet */
	rpkt = (struct netpacket_r *)getbuf(netbufpool);
	if((int32)rpkt == SYSERR) {
		restore(mask);
		freebuf((char *)pkt);
		return SYSERR;
	}

	/* Initialize the ethernet fields */
	memcpy(rpkt->net_ethsrc, iftab[0].if_hwucast, 6);
	memcpy(rpkt->net_ethdst, info.mcastaddr, 6);
	rpkt->net_ethtype = htons(ETH_TYPE_B);

	/* Initialize the radio header fields */
	rpkt->net_radfc = (RAD_FC_FT_DAT |
			   RAD_FC_AR |
			   RAD_FC_DAM3 |
			   RAD_FC_FV2 |
			   RAD_FC_SAM3);

	rpkt->net_radseq = ifptr->if_seq++;

	memcpy(rpkt->net_radsrc, ifptr->if_hwucast, 8);

	iplen = 40 + ntohs(pkt->net_iplen);

	/* Check the IP destination to determine next step */

	if(isipmc(pkt->net_ipdst)) { /* Multicast IP */
		memset(rpkt->net_raddst, 0xff, 8);
		memcpy(rpkt->net_raddata, pkt, iplen);
	}
	else if(isipllu(pkt->net_ipdst)) { /* Link-local IP */

		/* Resolve the IP address into L2 address */
		memcpy(rpkt->net_raddst, pkt->net_ipdst+8, 8);
		if(rpkt->net_raddst[0] & 0x02) {
			rpkt->net_raddst[0] &= 0xfd;
		}
		else {
			rpkt->net_raddst[0] |= 0x02;
		}
		memcpy(rpkt->net_raddata, pkt, iplen);
	}
	else { /* Off-link IP address */

		if(rpltab[0].root) {
			ip_send_rpl_lbr(pkt);
			restore(mask);
			return OK;
		}

		ncindex = nd_ncfind(pkt->net_ipdst);
		if(ncindex != SYSERR) {

			#ifdef DEBUG_IP
			kprintf("ip_send_rpl: destination is neighbor\n");
			#endif
			memcpy(rpkt->net_raddst, nd_ncache[ncindex].nc_hwaddr, 8);
			iplen = 40 + ntohs(pkt->net_iplen);
			memcpy(rpkt->net_raddata, pkt, iplen);
		}
		else {
			/* Next hop must be a link-local IP */
			if(!isipllu(nxthop)) {
				restore(mask);
				freebuf((char *)pkt);
				return SYSERR;
			}

			/* Original IP packet will be encapsulated in an IP packet */

			/* Outer IP header fields */
			npkt = (struct netpacket *)rpkt->net_raddata;
			memset(npkt, 0, 40);
			npkt->net_ipvtch = 0x60;
			npkt->net_iplen = 8 + iplen;
			npkt->net_ipnh = IP_EXT_HBH;
			npkt->net_iphl = 0xff;
			memcpy(npkt->net_ipsrc, ifptr->if_ipucast[0].ipaddr, 16);
			memcpy(npkt->net_ipdst, nxthop, 16);

			/* Hop-by-hop RPL extension header */
			exptr = (struct ip_ext_hdr *)npkt->net_ipdata;
			exptr->ipext_nh = IP_IP;
			exptr->ipext_len = 0;
			exptr->ipext_optype = 0x63;
			exptr->ipext_optlen = 4;
			exptr->ipext_f = 0;
			exptr->ipext_r = 0;
			exptr->ipext_o = 0;
			exptr->ipext_rplinsid = 0;
			exptr->ipext_sndrank = 0;

			/* Copy the original IP Packet */
			memcpy(npkt->net_ipdata+8, pkt, iplen);

			/* Resolve L2 address from nexthop IP address */
			memcpy(rpkt->net_raddst, nxthop+8, 8);
			if(rpkt->net_raddst[0] & 0x02) {
				rpkt->net_raddst[0] &= 0xfd;
			}
			else {
				rpkt->net_raddst[0] |= 0x02;
			}

			iplen += 48;
		}
	}

	#ifdef DEBUG_IP
	int32	i;
	kprintf("ip_send_rpl: sending radio packet\n");
	for(i = 0; i < 14 + 24 + 40 + 8 + 40; i++) {
		kprintf("%02x ", *((byte *)rpkt + i));
	}
	kprintf("\n");
	#endif

	write(RADIO0, (char *)rpkt, 14 + 24 + iplen);

	freebuf((char *)pkt);
	freebuf((char *)rpkt);

	restore(mask);
	return OK;
}

/*------------------------------------------------------------------------
 * ip_hton  -  Convert IP header fields in network byte order
 *------------------------------------------------------------------------
 */
void	ip_hton (
		struct	netpacket *pkt
		)
{
	pkt->net_iplen = htons(pkt->net_iplen);
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

/*------------------------------------------------------------------------
 * colon2ip  -  Convert string IP to binary
 *------------------------------------------------------------------------
 */
int32	colon2ip (
		char	*ipstr,
		byte	ipaddr[]
		)
{
	char	*p;
	byte	b;
	int32	i, j;

	if(strlen(ipstr) !=39) {
		return SYSERR;
	}

	p = ipstr;
	for(i = 0; i < 16; i++) {

		b = 0;
		for(j = 0; j < 2; j++) {
			b = b * 16;
			if(*p >= '0' && *p <= '9') {
				b += *p - '0';
			}
			else if(*p >= 'a' &&  *p <= 'f') {
				b += 10 + (*p - 'a');
			}
			else if(*p >= 'A' && *p <= 'F') {
				b += 10 + (*p - 'A');
			}
			else {
				return SYSERR;
			}
			p++;
		}
		ipaddr[i] = b;
		if(i > 0 && i < 15 && ((i+1) % 2 == 0)) {
			if(*p != ':') {
				return SYSERR;
			}
			p++;
		}
	}

	return OK;
}
