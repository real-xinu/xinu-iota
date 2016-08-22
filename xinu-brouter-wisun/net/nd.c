/* nd.c - nd_init, nd_in */

#include <xinu.h>

/* Neighbor Cache */
struct	nd_ncentry nd_ncache[ND_NCACHE_SIZE];

/*------------------------------------------------------------------------
 * nd_init  -  Initialize ND relate data structures
 *------------------------------------------------------------------------
 */
void	nd_init(void) {

	struct	nd_ncentry *ncptr;	/* Neighbor cache entry	*/
	intmask	mask;			/* Interrupt mask	*/
	int32	i;			/* Index variable	*/

	mask = disable();

	/* Initialize the neighbor cache */
	for(i = 0; i < ND_NCACHE_SIZE; i++) {
		ncptr = &nd_ncache[i];
		memset(ncptr, NULLCH, sizeof(struct nd_ncentry));
		ncptr->nc_state = NC_STATE_FREE;
		ncptr->nc_pqhead = ncptr->nc_pqtail = ncptr->nc_pqcount = 0;
	}

	restore(mask);

	/* Create a neighbor discovery timer process */
	resume(create(nd_timer, 8192, 500, "nd_timer", 0, NULL));
}

/*------------------------------------------------------------------------
 * nd_newipucast  -  Handle a new IP address assigned to an interface
 *------------------------------------------------------------------------
 */
int32	nd_newipucast (
		int32	iface,		/* Interface index	*/
		byte	ipaddr[]	/* IP address		*/
		)
{
	struct	netiface *ifptr;	/* Network interface pointer	*/
	intmask	mask;			/* Interrupt mask		*/
	int32	i;			/* Index variable		*/

	/* Verify that the interface index is valid */
	if(iface < 0 || iface >= NIFACES) {
		return SYSERR;
	}

	ifptr = &iftab[iface];

	mask = disable();

	/* If we cannot add any more multicast addresses, return error */
	i = ifptr->if_nipmcast;
	if(i >= IF_MAX_NIPMCAST) {
		restore(mask);
		return SYSERR;
	}

	/* Generate a solicited-node multicast address */
	memcpy(ifptr->if_ipmcast[i].ipaddr, ip_nd_snmcprefix, 16);
	memcpy(ifptr->if_ipmcast[i].ipaddr+13, ipaddr+13, 3);
	ifptr->if_nipmcast++;

	restore(mask);

	return OK;
}

/*------------------------------------------------------------------------
 * nd_ncfind  -  Find an entry in the neighbor cache (assumes intr off)
 *------------------------------------------------------------------------
 */
int32	nd_ncfind (
		byte	*ipaddr		/* IP address	*/
		)
{
	struct	nd_ncentry *ncptr;	/* Neighbor cache entry	*/
	int32	i;			/* Index variable	*/

	/* Search for the entry matching the IP address */
	for(i = 0; i < ND_NCACHE_SIZE; i++) {

		ncptr = &nd_ncache[i];

		if(ncptr->nc_state == NC_STATE_FREE) {
			continue;
		}

		if(!memcmp(ncptr->nc_ipaddr, ipaddr, 16)) {
			return i;
		}
	}

	return SYSERR;
}

/*------------------------------------------------------------------------
 * nd_ncupdate  -  Update a neighbor cache entry
 *------------------------------------------------------------------------
 */
int32	nd_ncupdate (
		byte	ipaddr[],	/* IP address		*/
		byte	hwaddr[],	/* Hardware address	*/
		int32	isrouter,	/* Is router?		*/
		bool8	irvalid		/* isrouter valid?	*/
		)
{
	struct	nd_ncentry *ncptr;	/* Neighbor cache entry	*/
	int32	ncindex;		/* Neighbor cache index	*/
	intmask	mask;			/* Interrupt mask	*/

	mask = disable();

	/* If no entry found for this IP, return error */
	ncindex = nd_ncfind(ipaddr);
	if(ncindex == SYSERR) {
		restore(mask);
		return SYSERR;
	}

	ncptr = &nd_ncache[ncindex];

	/* If the hardware address is different, make the entry STALE */
	if(memcmp(ncptr->nc_hwaddr, hwaddr, iftab[ncptr->nc_iface].if_halen)) {
		memcpy(ncptr->nc_hwaddr, hwaddr, iftab[ncptr->nc_iface].if_halen);
		ncptr->nc_rstate = NC_RSTATE_STL;
	}

	/* Change the isrouter flag if valid */
	if(irvalid) {
		ncptr->nc_isrouter = isrouter;
	}

	restore(mask);
	return OK;
}

/*------------------------------------------------------------------------
 * nd_ncnew  -  Add a new entry to the neighbor cache (assumes intr off)
 *------------------------------------------------------------------------
 */
int32	nd_ncnew (
		byte	ipaddr[],	/* IP address		*/
		byte	hwaddr[],	/* Hardware address	*/
		int32	iface,		/* Interface index	*/
		int32	rstate,		/* Reachable state	*/
		int32	isrouter	/* isrouter?		*/
		)
{
	struct	nd_ncentry *ncptr;	/* Neighbor cache entry	*/
	int32	i;			/* Index variable	*/

	/* Search for an empty neighbor cache entry */
	for(i = 0; i < ND_NCACHE_SIZE; i++) {

		ncptr = &nd_ncache[i];

		if(ncptr->nc_state == NC_STATE_FREE) {
			break;
		}
	}

	if(i >= ND_NCACHE_SIZE) {
		kprintf("Neighbor cache full\n");
		return SYSERR;
	}

	memset(ncptr, NULLCH, sizeof(struct nd_ncentry));

	/* Initialize the interface, reachable state and IP address */
	ncptr->nc_iface = iface;
	ncptr->nc_rstate = rstate;
	memcpy(ncptr->nc_ipaddr, ipaddr, 16);

	/* If hardware address is provided, add it */
	if(hwaddr) {
		memcpy(ncptr->nc_hwaddr, hwaddr, iftab[iface].if_halen);
	}

	ncptr->nc_isrouter = isrouter;

	if(ncptr->nc_rstate == NC_RSTATE_INC) {
		ncptr->nc_texpire = iftab[ncptr->nc_iface].if_nd_retranstime;
	}
	else {
		ncptr->nc_texpire = iftab[iface].if_nd_reachtime;
	}

	ncptr->nc_state = NC_STATE_USED;

	//kprintf("Added a new NC entry for "); ip_printaddr(ipaddr);
	//kprintf("\n");

	return i;
}

/*------------------------------------------------------------------------
 * nd_in  -  Handle incoming ND packets
 *------------------------------------------------------------------------
 */
void	nd_in (
		struct	netpacket *pkt	/* Packet buffer pointer	*/
	      )
{
	//kprintf("Incoming nd packet.\n");

	switch(pkt->net_ictype) {

	case ICMP_TYPE_NS:
		nd_in_ns(pkt);
		break;

	case ICMP_TYPE_NA:
		nd_in_na(pkt);
		break;

	default:
		break;
	}
}

/*------------------------------------------------------------------------
 * nd_in_ns  -  Handle neighbor solicitation message
 *------------------------------------------------------------------------
 */
void	nd_in_ns (
		struct	netpacket *pkt	/* Packet buffer pointer	*/
		)
{
	struct	nd_nbrsol *nsmsg;
	struct	nd_nbradv *namsg;
	struct	nd_opt *ndopt;
	struct	netiface *ifptr;
	byte	*ipdst;
	int32	nalen;
	int32	ncindex;
	intmask	mask;
	int32	i;

	nsmsg = (struct nd_nbrsol *)pkt->net_icdata;

	if(pkt->net_iccode != 0) {
		return;
	}

	if(pkt->net_iplen < 24) {
		return;
	}

	if(isipmc(nsmsg->nd_tgtaddr)) {
		return;
	}

	if(isipunspec(pkt->net_ipsrc)) {
		if(memcmp(pkt->net_ipdst, ip_nd_snmcprefix, 13)) {
			return;
		}
	}

	mask = disable();
	ifptr = &iftab[pkt->net_iface];
	for(i = 0; i < ifptr->if_nipucast; i++) {
		if(!memcmp(ifptr->if_ipucast[i].ipaddr, nsmsg->nd_tgtaddr, 16)) {
			break;
		}
	}
	if(i >= ifptr->if_nipucast) {
		restore(mask);
		return;
	}
	restore(mask);

	//kprintf("incoming ns msg for"); ip_printaddr(nsmsg->nd_tgtaddr);
	//kprintf("\n");

	ndopt = (struct nd_opt *)nsmsg->nd_opts;

	while((char *)ndopt < ((char *)pkt->net_ipdata + pkt->net_iplen)) {

		switch(ndopt->ndopt_type) {

		case NDOPT_TYPE_SLLA:
			if(isipunspec(pkt->net_ipsrc)) {
				return;
			}

			//kprintf("sllao: ");
			//ip_printaddr(ndopt->ndopt_addr);
			//kprintf("\n");

			mask = disable();

			ncindex = nd_ncupdate(pkt->net_ipsrc,
					      ndopt->ndopt_addr,
					      0,
					      FALSE);
			//kprintf("nd_in_ns: ncindex %d\n", ncindex);

			if(ncindex == SYSERR) {
				ncindex = nd_ncnew(pkt->net_ipsrc,
						   ndopt->ndopt_addr,
						   pkt->net_iface,
						   NC_RSTATE_STL,
						   0);
			}

			restore(mask);

			ndopt = (struct nd_opt *)((char *)ndopt + (ndopt->ndopt_len * 8));
			break;

		default:
			ndopt = (struct nd_opt *)((char *)ndopt + (ndopt->ndopt_len * 8));
			break;
		}
	}

	//kprintf("nd_in_ns: sending na\n");
	nalen = sizeof(struct nd_nbradv) + 8;

	namsg = (struct nd_nbradv *)getmem(nalen);

	if((int32)namsg == SYSERR) {
		return;
	}

	memset(namsg, 0, nalen);
	namsg->nd_rso = ND_NA_S | ND_NA_O;
	memcpy(namsg->nd_tgtaddr, nsmsg->nd_tgtaddr, 16);

	mask = disable();

	ifptr = &iftab[pkt->net_iface];

	ndopt = (struct nd_opt *)namsg->nd_opts;
	ndopt->ndopt_type = NDOPT_TYPE_TLLA;
	ndopt->ndopt_len = 1;
	memcpy(ndopt->ndopt_addr, ifptr->if_hwucast, ifptr->if_halen);

	//kprintf("nd_in_ns: calling icmp_send\n");
	if(isipunspec(pkt->net_ipsrc)) {
		ipdst = ip_allnodesmc;
	}
	else {
		ipdst = pkt->net_ipsrc;
	}

	icmp_send(ICMP_TYPE_NA, 0, ipdst, namsg, nalen, pkt->net_iface);

	restore(mask);

}

/*------------------------------------------------------------------------
 * nd_in_na  -  Handle an incoming neighbor advertisement message
 *------------------------------------------------------------------------
 */
void	nd_in_na (
		struct	netpacket *pkt
		)
{
	struct	nd_nbradv *namsg;
	struct	nd_ncentry *ncptr;
	struct	netiface *ifptr;
	struct	netpacket_e *epkt;
	struct	nd_opt *ndopt;
	struct	nd_opt *tllao;
	int32	ncindex;
	bool8	l2updated;
	intmask	mask;

	namsg = (struct nd_nbradv *)pkt->net_icdata;

	if(pkt->net_iphl < 255) {
		return;
	}

	if(pkt->net_iccode != 0) {
		return;
	}

	if(pkt->net_iplen < 24) {
		return;
	}

	if(isipmc(namsg->nd_tgtaddr)) {
		return;
	}

	if(isipmc(pkt->net_ipdst) && (namsg->nd_rso & ND_NA_S)) {
		return;
	}

	mask = disable();

	ncindex = nd_ncfind(namsg->nd_tgtaddr);
	if(ncindex == SYSERR) {
		restore(mask);
		return;
	}

	ncptr = &nd_ncache[ncindex];

	ifptr = &iftab[ncptr->nc_iface];

	ndopt = (struct nd_opt *)namsg->nd_opts;

	tllao = NULL;

	while((char *)ndopt < (char *)pkt->net_ipdata + pkt->net_iplen) {

		switch(ndopt->ndopt_type) {

		case NDOPT_TYPE_TLLA:
			tllao = ndopt;

			//kprintf("tllao: "); ip_printaddr(ndopt->ndopt_addr);
			//kprintf("\n");

			ndopt = (struct nd_opt *)((char *)ndopt + ndopt->ndopt_len * 8);
			break;

		default:
			ndopt = (struct nd_opt *)((char *)ndopt + ndopt->ndopt_len * 8);
		}
	}

	//kprintf("nd_in_na: done processing options\n");

	if(ncptr->nc_rstate == NC_RSTATE_INC) {

		if(tllao == NULL) {
			restore(mask);
			return;
		}

		memcpy(ncptr->nc_hwaddr, tllao->ndopt_addr, ifptr->if_halen);

		if(namsg->nd_rso & ND_NA_S) {
			ncptr->nc_rstate = NC_RSTATE_RCH;
			ncptr->nc_texpire = ifptr->if_nd_reachtime;
		}
		else {
			ncptr->nc_rstate = NC_RSTATE_STL;
		}

		ncptr->nc_isrouter = (namsg->nd_rso & ND_NA_R);

		while(ncptr->nc_pqcount > 0) {
			//kprintf("nd_in_na: sending queued pkt\n");
			pkt = ncptr->nc_pktq[ncptr->nc_pqtail];
			ncptr->nc_pqtail++;
			if(ncptr->nc_pqtail >= NC_PKTQ_SIZE) {
				ncptr->nc_pqtail = 0;
			}
			ncptr->nc_pqcount--;

			if(ifptr->if_type == IF_TYPE_ETH) {

				epkt = (struct netpacket_e *)getbuf(netbufpool);
				if((int32)epkt == SYSERR) {
					continue;
				}

				memcpy(epkt->net_ethsrc, ifptr->if_hwucast, ifptr->if_halen);
				memcpy(epkt->net_ethdst, ncptr->nc_hwaddr, ifptr->if_halen);
				epkt->net_ethtype = htons(ETH_IPV6);

				memcpy(epkt->net_ethdata, (char *)pkt, 40 + ntohs(pkt->net_iplen));

				write(ETHER0, (char *)epkt, 14 + 40 + ntohs(pkt->net_iplen));

				freebuf((char *)pkt);
				freebuf((char *)epkt);
			}
		}

		restore(mask);
		return;
	}

	if(tllao && !(namsg->nd_rso & ND_NA_O) &&
		memcmp(ncptr->nc_hwaddr, tllao->ndopt_addr, ifptr->if_halen)) {
			if(ncptr->nc_rstate == NC_RSTATE_RCH) {
				ncptr->nc_rstate = NC_RSTATE_STL;
			}

		restore(mask);
		return;
	}

	l2updated = FALSE;
	if(tllao && memcmp(ncptr->nc_hwaddr, tllao->ndopt_addr, ifptr->if_halen)) {
		memcpy(ncptr->nc_hwaddr, tllao->ndopt_addr, ifptr->if_halen);
		l2updated = TRUE;
	}

	if(namsg->nd_rso & ND_NA_S) {
		ncptr->nc_rstate = NC_RSTATE_RCH;
		ncptr->nc_texpire = ifptr->if_nd_reachtime;
	}
	else {
		if(l2updated) {
			ncptr->nc_rstate = NC_RSTATE_STL;
		}
	}

	restore(mask);
	return;
}

/*------------------------------------------------------------------------
 * nd_send_ns  -  Send a neighbor solicitation message (assumes intr. off)
 *------------------------------------------------------------------------
 */
int32	nd_send_ns (
		int32	ncindex
		)
{
	struct	nd_ncentry *ncptr;
	struct	nd_nbrsol *nsmsg;
	struct	nd_opt *ndopt;
	byte	ipdst[16];
	int32	nslen;

	ncptr = &nd_ncache[ncindex];

	nslen = sizeof(struct nd_nbrsol) + 8;

	nsmsg = (struct	nd_nbrsol *)getmem(nslen);
	if((int32)nslen == SYSERR) {
		return SYSERR;
	}

	memset(nsmsg, 0, nslen);
	memcpy(nsmsg->nd_tgtaddr, ncptr->nc_ipaddr, 16);

	ndopt = (struct nd_opt *)nsmsg->nd_opts;
	ndopt->ndopt_type = NDOPT_TYPE_SLLA;
	ndopt->ndopt_len = 1;
	memcpy(ndopt->ndopt_addr, iftab[ncptr->nc_iface].if_hwucast,
			iftab[ncptr->nc_iface].if_halen);

	if(ncptr->nc_rstate == NC_RSTATE_INC) {
		memcpy(ipdst, ip_nd_snmcprefix, 16);
		memcpy(ipdst+13, ncptr->nc_ipaddr+13, 3);
	}
	else {
		memcpy(ipdst, ncptr->nc_ipaddr, 16);
	}

	icmp_send(ICMP_TYPE_NS, 0, ipdst, nsmsg, nslen, ncptr->nc_iface);

	return OK;
}

/*------------------------------------------------------------------------
 * nd_resolve  -  Resolve an IP address to an L2 address
 *------------------------------------------------------------------------
 */
int32	nd_resolve (
		byte	ipaddr[],
		int32	iface,
		void	*pkt
		)
{
	struct	nd_ncentry *ncptr;
	int32	ncindex;
	intmask	mask;

	mask = disable();

	ncindex = nd_ncfind(ipaddr);
	if(ncindex != SYSERR) {
		restore(mask);
		return SYSERR;
	}

	ncindex = nd_ncnew(ipaddr, NULL, iface, NC_RSTATE_INC, 0);
	if(ncindex == SYSERR) {
		restore(mask);
		return SYSERR;
	}

	ncptr = &nd_ncache[ncindex];

	ncptr->nc_pktq[ncptr->nc_pqhead++] = pkt;
	if(ncptr->nc_pqhead >= NC_PKTQ_SIZE) {
		ncptr->nc_pqhead = 0;
	}
	if(ncptr->nc_pqcount < NC_PKTQ_SIZE) {
		ncptr->nc_pqcount++;
	}
	else {
		ncptr->nc_pqcount = NC_PKTQ_SIZE;
	}

	nd_send_ns(ncindex);

	ncptr->nc_retries++;

	restore(mask);
	return OK;
}

/*------------------------------------------------------------------------
 * nd_timer  -  Timer process to manage neighbor cache entries
 *------------------------------------------------------------------------
 */
process	nd_timer (void) {

	struct	nd_ncentry *ncptr;
	intmask	mask;
	int32	i;

	while(TRUE) {

		mask = disable();

		for(i = 0; i < ND_NCACHE_SIZE; i++) {

			ncptr = &nd_ncache[i];

			if(ncptr->nc_state == NC_STATE_FREE) {
				continue;
			}

			switch(ncptr->nc_rstate) {

			case NC_RSTATE_INC:
				if(--ncptr->nc_texpire <= 0) {
					if(ncptr->nc_retries >= ND_MAX_MULTICAST_SOLICIT) {
						while(ncptr->nc_pqcount > 0) {
							freebuf(ncptr->nc_pktq[ncptr->nc_pqtail++]);
							if(ncptr->nc_pqtail >= NC_PKTQ_SIZE) {
								ncptr->nc_pqtail = 0;
							}
							ncptr->nc_pqcount--;
						}
						ncptr->nc_state = NC_STATE_FREE;
					}
					else {
						ncptr->nc_retries++;
						ncptr->nc_texpire = iftab[ncptr->nc_iface].if_nd_retranstime;
						nd_send_ns(i);
					}
				}
				break;

			case NC_RSTATE_RCH:
				if(--ncptr->nc_texpire <= 0) {
					kprintf("nd_timer: "); ip_printaddr(ncptr->nc_ipaddr);
					kprintf(" changed state to STALE\n");
					ncptr->nc_rstate = NC_RSTATE_STL;
				}
				break;

			case NC_RSTATE_DLY:
				if(--ncptr->nc_texpire <= 0) {
					kprintf("nd_timer: "); ip_printaddr(ncptr->nc_ipaddr);
					kprintf(" changed state to PROBE\n");
					ncptr->nc_rstate = NC_RSTATE_PRB;
					ncptr->nc_texpire = iftab[ncptr->nc_iface].if_nd_retranstime;
					nd_send_ns(i);
				}
				break;

			case NC_RSTATE_PRB:
				if(--ncptr->nc_texpire <= 0) {
					if(ncptr->nc_retries >= ND_MAX_UNICAST_SOLICIT) {
						while(ncptr->nc_pqcount > 0) {
							freebuf(ncptr->nc_pktq[ncptr->nc_pqtail++]);
							if(ncptr->nc_pqtail >= NC_PKTQ_SIZE) {
								ncptr->nc_pqtail = 0;
							}
							ncptr->nc_pqcount--;
						}
						ncptr->nc_state = NC_STATE_FREE;
					}
					else {
						ncptr->nc_retries++;
						ncptr->nc_texpire = iftab[ncptr->nc_iface].if_nd_retranstime;
						nd_send_ns(i);
					}
				}
				break;

			}
		}

		restore(mask);

		sleepms(1);
	}

	return OK;
}
