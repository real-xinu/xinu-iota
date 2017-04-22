/* rpl.c  -  rpl_init */

#include <xinu.h>

//#define	DEBUG_RPL	1

struct	rplentry rpltab[NRPL];

/*------------------------------------------------------------------------
 * rpl_init  -  Initialize RPl related data structures
 *------------------------------------------------------------------------
 */
void	rpl_init (
		int32	iface
		)
{

	struct	rplentry *rplptr;	/* Pointer to RPL entry	*/
	struct	netiface *ifptr;	/* Pointer to net iface	*/
	intmask	mask;			/* Interrupt mask	*/
	int32	i;			/* Index variable	*/

	ifptr = &iftab[1];

	memcpy(ifptr->if_ipmcast[ifptr->if_nipmcast].ipaddr, ip_allrplnodesmc,
					16);
	ifptr->if_nipmcast++;

	#ifdef ROOT
		rpl_lbr_init();
		return;
	#endif

	mask = disable();

	for(i = 0; i < NRPL; i++) {

		rplptr = &rpltab[i];

		memset(rplptr, 0, sizeof(struct rplentry));

		rplptr->cur_min_path_cost = RPL_MAX_PATH_COST;
		rplptr->prefparent = -1;

		rplptr->state = RPL_STATE_FREE;
	}

	resume(rplptr->timerproc = create(rpl_timer, 8192, NETPRIO, "rpl timer", 0, NULL));

	restore(mask);
}

/*------------------------------------------------------------------------
 * rpl_in  -  Handle incoming RPL packets
 *------------------------------------------------------------------------
 */
void	rpl_in (
		struct	netpacket *pkt	/* Packet buffer pointer	*/
	       )
{
	switch(pkt->net_iccode) {

	case ICMP_CODE_RPL_DIS:
		rpl_in_dis(pkt);
		break;

	case ICMP_CODE_RPL_DIO:
		rpl_in_dio(pkt);
		break;

	case ICMP_CODE_RPL_DAO:
		rpl_in_dao(pkt);
		break;
	
	case ICMP_CODE_RPL_DAOK:
		rpl_in_daok(pkt);
		break;
	}
}

/*------------------------------------------------------------------------
 * rpl_in_dis  -  Handle an incoming DIS message
 *------------------------------------------------------------------------
 */
void	rpl_in_dis (
		struct	netpacket *pkt	/* Packet buffer	*/
		)
{
	struct	rplentry *rplptr;	/* RPL entry		*/
	struct	rpl_dis *dismsg;	/* DIS message pointer	*/
	struct	rplopt_solinfo *siopt;	/* Sol. information opt	*/
	byte	*rplopt;		/* Generic RPL opt ptr	*/
	intmask	mask;			/* Interrupt mask	*/
	int32	msglen;			/* Message length	*/
	int32	i;			/* Index variable	*/

	#ifdef DEBUG_RPL
	kprintf("rpl_in_dis: incoming DIS\n");
	#endif

	/* DIS message length */
	msglen = pkt->net_iplen - 4;

	dismsg = (struct rpl_dis *)pkt->net_icdata;

	rplopt = dismsg->rpl_opts;

	/* Parse the options in the message */
	siopt = NULL;
	while(rplopt < (pkt->net_icdata + msglen)) {
	
		switch(*rplopt) {

		case RPLOPT_TYPE_SI:
			siopt = (struct rplopt_solinfo *)rplopt;
			rplopt += siopt->rplopt_len + 2;
			break;

		case RPLOPT_TYPE_P1:
			rplopt++;
			break;

		default:
			rplopt += (*(rplopt+1)) + 2;
			break;
		}
	}

	/* If no solicited information option present, fail */
	if(siopt == NULL) {
		#ifdef DEBUG_RPL
		kprintf("rpl_in_dis: Solicitation option not present\n");
		#endif
		return;
	}

	mask = disable();

	/* Search for a RPl table entry matching the incoming DIS */
	for(i = 0; i < NRPL; i++) {

		rplptr = &rpltab[i];

		if(rplptr->state == RPL_STATE_FREE) {
			continue;
		}

		if(!rplptr->daok) {
			continue;
		}

		if(siopt->rplopt_i && (rplptr->rplinsid != siopt->rplopt_insid)) {
			continue;
		}

		if(siopt->rplopt_d && memcmp(siopt->rplopt_dodagid, rplptr->dodagid, 16)) {
			continue;
		}

		if(siopt->rplopt_v && (siopt->rplopt_vers != rplptr->vers)) {
			continue;
		}

		#ifdef DEBUG_RPL
		kprintf("rpl_in_dis: sending DIO\n");
		#endif

		if(!isipmc(pkt->net_ipdst)) {
			/* Send a DIO message to sender */
			rpl_send_dio(i, pkt->net_ipsrc);
		}
		else {
			/* Reset the trickle timer */
			trickle_reset(rplptr->trickle);
			//rpl_send_dio(i, ip_allrplnodesmc);
		}
	}

	restore(mask);
}

/*------------------------------------------------------------------------
 * rpl_in_dio  -  Handle an incoming DIO message
 *------------------------------------------------------------------------
 */
void	rpl_in_dio (
		struct	netpacket *pkt	/* Packet buffer	*/
		)
{
	struct	netiface *ifptr;	/* Network interface	*/
	struct	rplentry *rplptr;	/* RPl entry		*/
	struct	rpl_dio *diomsg;	/* DIo message		*/
	struct	rplopt_pi *piopt;	/* Prefix Info. option	*/
	struct	rplopt_pi *piopt_r;	/* PI opt. with R Flag	*/
	struct	rplopt_ri *riopt;	/* Route Info. option	*/
	struct	rplopt_dodagconfig *dcopt;
					/* DODAG Config. option	*/
	byte	*rplopt;		/* Generic RPL option	*/
	intmask	mask;			/* Interrupt mask	*/
	int32	msglen;			/* Message length	*/
	int32	found, free;		/* Flags		*/
	bool8	newrpl;			/* NewRPL entry?	*/
	int32	a, b;			/* Temporary values	*/
	int32	i;			/* Index variable	*/
	byte	hwaddr[8];

	#ifdef DEBUG_RPL
	kprintf("rpl_in_dio: ...\n");
	#endif

	newrpl = FALSE;

	diomsg = (struct rpl_dio *)pkt->net_icdata;

	/* DODAG must be grounded and mode of operation = 1 */
	if(diomsg->rpl_g != 1 || (diomsg->rpl_mop != RPL_DIO_MOP1)) {
		#ifdef DEBUG_RPL
		kprintf("rpl_in_dio: g = %d or mop = %d\n", diomsg->rpl_g, diomsg->rpl_mop);
		#endif
		return;
	}

	/* Let's see if we are part of this DODAG */

	mask = disable();

	found = -1;
	free = -1;
	for(i = 0; i < NRPL; i++) {

		rplptr = &rpltab[i];

		if(rplptr->state == RPL_STATE_FREE) {
			free = free == -1 ? i : free;
			continue;
		}

		if((diomsg->rpl_insid == rplptr->rplinsid) &&
		   (!memcmp(diomsg->rpl_dodagid, rplptr->dodagid, 16))) {

			if(diomsg->rpl_vers < rplptr->vers) {
				restore(mask);
				return;
			}

			if(diomsg->rpl_vers == rplptr->vers) {
				found = i;
				break;
			}

			//TODO: handle increased version
		}
	}

	if(found != -1) { /* Found a matching RPL instance */

		rplptr = &rpltab[found];

		/* Check if the DIO is from one of our neighbors */
		for(i = 0; i < rplptr->nneighbors; i++) {
			if(!memcmp(rplptr->neighbors[i].lluip, pkt->net_ipsrc, 16)) {
				break;
			}
		}
		if(i >= rplptr->nneighbors) { /* This is a new neighbor */

			/* Check if we can store a new neighbor */
			if(rplptr->nneighbors >= MAX_RPL_NEIGHBORS) {
				restore(mask);
				return;
			}

			#ifdef DEBUG_RPL
			kprintf("rpl_in_dio: neighbor: ");
			ip_printaddr(pkt->net_ipsrc);
			kprintf(" rank %d, %d...", RPL_DAGRank(rplptr, ntohs(diomsg->rpl_rank)), RPL_DAGRank(rplptr, rplptr->rank));
			#endif
			if(RPL_DAGRank(rplptr, ntohs(diomsg->rpl_rank)) >= RPL_DAGRank(rplptr, rplptr->rank)) {
				//kprintf("ignoring\n");
				restore(mask);
				return;
			}
			//kprintf("considering\n");

			/* For a new neighbor, we will need a PI option with R flag set */

			rplopt = diomsg->rpl_opts;
			piopt_r = NULL;
			while(rplopt < (pkt->net_ipdata + pkt->net_iplen)) {

				switch(*rplopt) {

				case RPLOPT_TYPE_PI:
					piopt = (struct rplopt_pi *)rplopt;
					if(piopt->rplopt_r == 1) {
						piopt_r = piopt;
					}

				default:
					rplopt += (*(rplopt+1)) + 2;
					break;
				}
			}

			/* Cannot find an eligible PI option, fail */
			if(piopt_r == NULL) {
				restore(mask);
				return;
			}

			/* Add the neighbor in the neighbor set */
			i = rplptr->nneighbors;
			memcpy(rplptr->neighbors[i].lluip,
					pkt->net_ipsrc, 16);
			memcpy(rplptr->neighbors[i].gip,
					piopt->rplopt_prefix, 16);
			rplptr->neighbors[i].rank = ntohs(diomsg->rpl_rank);

			//TODO: compute path cost here by replacing 128 by etx of this neighbor
			/*
			memcpy(hwaddr, &rplptr->neighbors[i].lluip[8], 8);
			if(hwaddr[0] & 0x02) {
				hwaddr[0] &= ~0x02;
			}
			else {
				hwaddr[0] |= 0x02;
			}
			for(i = 0; i < NBRTAB_SIZE; i++) {
				if(radtab[0].nbrtab[i].state == NBR_STATE_FREE) {
					continue;
				}
				if(!memcmp(radtab[0].nbrtab[i].hwaddr, hwaddr, 8)) {
					break;
				}
			}
			if(i >= NBRTAB_SIZE) {
				rplptr->neighbors[i].pathcost = rplptr->neighbors[i].rank + 128;
			}
			else {
				rplptr->neighbors[i].pathcost = rplptr->neighbors[i].rank + 128*radtab[0].nbrtab[i].etx;
			}

			a = rplptr->neighbors[i].pathrank;
			b = rplptr->neighbors[i].rank + rplptr->minhoprankinc;
			rplptr->neighbors[i].pathrank = (a > b) ? a : b;
			*/

			rplptr->neighbors[i].parent = FALSE;

			rplptr->nneighbors++;

			nd_regaddr(rplptr->neighbors[i].lluip, rplptr->iface, rpl_parents_cb, 0, 0);

			//rpl_parents(found);

			if(rplptr->neighbors[i].parent == FALSE) {
				restore(mask);
				return;
			}
		}
		else { /* It is our neighbor */

			/* Check if the neighbor's rank has changed */
			if(rplptr->neighbors[i].rank != ntohs(diomsg->rpl_rank)) {
				kprintf("rpl_in_dio: PARENT WITH CHANGED RANK: %d %d\n", rplptr->neighbors[i].rank, ntohs(diomsg->rpl_rank));
				rpl_parents(found);
			}
			restore(mask);
			return;
		}
	}
	else { /* This is an unknown RPL instance */

		/* If there are no free entries in RPL table, fail */
		if(free == -1) {
			restore(mask);
			return;
		}

		#ifdef DEBUG_RPL
		kprintf("New DODAG!\n");
		#endif

		/* Look for the DODAG Config. option */
		dcopt = NULL;
		piopt_r = NULL;
		rplopt = diomsg->rpl_opts;
		while(rplopt < (pkt->net_ipdata + pkt->net_iplen)) {

			switch(*rplopt) {

			case RPLOPT_TYPE_DC:
				dcopt = (struct rplopt_dodagconfig *)rplopt;
				rplopt += dcopt->rplopt_len + 2;
				break;

			case RPLOPT_TYPE_PI:
				piopt = (struct rplopt_pi *)rplopt;
				if(piopt->rplopt_r == 1) {
					piopt_r = piopt;
				}
				rplopt += piopt->rplopt_len + 2;
				break;
			
			default:
				rplopt += (*(rplopt + 1)) + 2;
				break;
			}
		}

		/* No DODAG Config option found, fail */
		if(dcopt == NULL) {
			#ifdef DEBUG_RPL
			kprintf("Cannot find DODAG Config option!\n");
			#endif
			restore(mask);
			return;
		}

		/* Enter the details of the RPL instance */
		rplptr = &rpltab[free];

		rplptr->iface = pkt->net_iface;
		rplptr->rplinsid = diomsg->rpl_insid;
		rplptr->vers = diomsg->rpl_vers;
		rplptr->mop = diomsg->rpl_mop;
		rplptr->dtsn = diomsg->rpl_dtsn;
		memcpy(rplptr->dodagid, diomsg->rpl_dodagid, 16);

		/* Indicate that this is a new RPL instance */
		newrpl = TRUE;
	}

	/* Parse the options now */

	rplopt = diomsg->rpl_opts;

	msglen = pkt->net_iplen - 4;

	while(rplopt < (pkt->net_icdata + msglen)) {

		switch(*rplopt) {

		/* PAD1 option ... */
		case RPLOPT_TYPE_P1:
			rplopt++;
			break;

		/* PADN option ... */
		case RPLOPT_TYPE_PN:
			rplopt += (*(rplopt + 2)) + 2;
			break;

		/* Prefix Information Option ... */
		case RPLOPT_TYPE_PI:
			piopt = (struct rplopt_pi *)rplopt;

			#ifdef DEBUG_RPL
			kprintf("rpl_in_dio: PIO\n");
			#endif
			/* If the A is not set, there's no use */
			if(piopt->rplopt_a == 0) {
				#ifdef DEBUG_RPL
				kprintf("rpl_in_dio: a = %d, r = %d\n", piopt->rplopt_a, piopt->rplopt_r);
				#endif
				rplopt += piopt->rplopt_len + 2;
				break;
			}

			/* Prefix length must be 64 */
			if(piopt->rplopt_preflen != 64) {
				#ifdef DEBUG_RPL
				kprintf("rpl_in_dio: pio prefix len != 64\n");
				#endif
				rplopt += piopt->rplopt_len + 2;
				break;
			}

			/* Look for the prefix in the prefix table */
			for(i = 0; i < rplptr->nprefixes; i++) {
				if(!memcmp(piopt->rplopt_prefix, rplptr->prefixes[i].prefix, 8)) {
					#ifdef DEBUG_RPL
					kprintf("rpl_in_dio: PIO prefix already present\n");
					#endif
					//TODO update the entry
					rplopt += piopt->rplopt_len + 2;
					break;
				}
			}
			if(i >= rplptr->nprefixes) { /* Not found ... */
				if(rplptr->nprefixes >= MAX_RPL_PREFIX) { /* Prefix table full */
					#ifdef DEBUG_RPL
					kprintf("rpl_in_dio: PIO prefix table full\n");
					#endif
					rplopt += piopt->rplopt_len + 2;
					break;
				}

				/* Add the new prefix in the table */
				i = rplptr->nprefixes;
				memcpy(rplptr->prefixes[i].prefix, piopt->rplopt_prefix, 8);
				rplptr->prefixes[i].prefixlen = 8;
				rplptr->prefixes[i].validlife = ntohl(piopt->rplopt_validlife);
				rplptr->prefixes[i].preflife = ntohl(piopt->rplopt_preflife);
				if(rplptr->prefixes[i].preflife == 0xffffffff) {
					rplptr->prefixes[i].texpire = 0xffffffff;
				}
				else {
					rplptr->prefixes[i].texpire = rplptr->prefixes[i].preflife * 1000;
				}
				rplptr->nprefixes++;

				/* Now use the prefix to configure an IPv6 address */
				ifptr = &iftab[pkt->net_iface];

				if(ifptr->if_nipucast >= IF_MAX_NIPUCAST) { /* No room for new IP address */
					rplopt += piopt->rplopt_len + 2;
					break;
				}

				i = ifptr->if_nipucast;
				memcpy(ifptr->if_ipucast[i].ipaddr, piopt->rplopt_prefix, 8);
				memcpy(&ifptr->if_ipucast[i].ipaddr[8], ifptr->if_hwucast, 8);
				if(ifptr->if_ipucast[i].ipaddr[8] & 0x02) {
					ifptr->if_ipucast[i].ipaddr[8] &= 0xfd;
				}
				else {
					ifptr->if_ipucast[i].ipaddr[8] |= 0x02;
				}
				ifptr->if_iptimes[i] = clktimems;
				ifptr->if_nipucast++;

				memcpy(rplptr->prefixes[rplptr->nprefixes-1].prefix+8,
						ifptr->if_ipucast[ifptr->if_nipucast-1].ipaddr+8, 8);

				#ifdef DEBUG_RPL
				kprintf("rpl_in_dio: New IP address %d for iface %d: ", i, pkt->net_iface);
				ip_printaddr(ifptr->if_ipucast[i].ipaddr);
				kprintf("\n");
				#endif
			}

			rplopt += piopt->rplopt_len + 2;
			if(piopt->rplopt_r == 0) {
				piopt = NULL;
			}
			break;

		/* Route Information option ... */
		case RPLOPT_TYPE_RI:
			riopt = (struct rplopt_ri *)rplopt;
			rplopt += riopt->rplopt_len + 2;
			break;

		/* DODAG Configuration option ... */
		case RPLOPT_TYPE_DC:
			dcopt = (struct rplopt_dodagconfig *)rplopt;

			rplptr->pcs = dcopt->rplopt_pcs;
			rplptr->diointdbl = dcopt->rplopt_diointdbl;
			rplptr->diointmin = dcopt->rplopt_diointmin;
			rplptr->dioredun = dcopt->rplopt_dioredun;
			rplptr->maxrankinc = ntohs(dcopt->rplopt_maxrankinc);
			rplptr->minhoprankinc = ntohs(dcopt->rplopt_minhoprankinc);
			rplptr->ocp = dcopt->rplopt_ocp;
			rplptr->deflife = dcopt->rplopt_deflife;
			rplptr->lifeunit = ntohs(dcopt->rplopt_lifeunit);

			rplopt += dcopt->rplopt_len + 2;
			break;

		default:
			rplopt += (*(rplopt + 1)) + 2;
			break;
		}
	}

	if(newrpl) { /* If this is a new RPL instance */

		if(piopt_r == NULL) {
			restore(mask);
			return;
		}

		rplptr->trickle = trickle_new(8, 268435456, 10);

		/* Make the sender our neighbor */

		memcpy(rplptr->neighbors[0].lluip, pkt->net_ipsrc, 16);
		memcpy(rplptr->neighbors[0].gip, piopt_r->rplopt_prefix, 16);
		#ifdef DEGUG_RPL
		kprintf("rpl_in_dio: newrpl info from ");
		ip_printaddr(piopt_r->rplopt_prefix);
		kprintf("\n");
		#endif
		rplptr->neighbors[0].rank = ntohs(diomsg->rpl_rank);
		/*
		//TODO compute path cost by replacing 128 by etx of neighbor
		rplptr->neighbors[0].pathcost = rplptr->neighbors[0].rank + 128;

		a = rplptr->neighbors[0].rank + rplptr->minhoprankinc;
		b = rplptr->neighbors[0].pathcost;
		rplptr->neighbors[0].pathrank = a > b ? a : b;
		*/

		rplptr->nneighbors = 1;

		/* This NS message is for registration as well as ETX */

		nd_regaddr(rplptr->neighbors[0].lluip, rplptr->iface, rpl_parents_cb, 0, 0);

		/* Compute the path control mask */
		rplptr->pcmask = 0x80;
		for(i = 0; i < rplptr->pcs; i++) {
			rplptr->pcmask >>= 1;
			rplptr->pcmask |= 0x80;
		}
		#ifdef DEBUG_RPL
		kprintf("rpl_in_dio: pcmask %02x\n", rplptr->pcmask);
		#endif

		rplptr->state = RPL_STATE_USED;

		//rpl_parents(free);
	}

	restore(mask);
}

/*------------------------------------------------------------------------
 * rpl_in_daok  -  Handle incoming DAO ACK message
 *------------------------------------------------------------------------
 */
void	rpl_in_daok (
		struct	netpacket *pkt	/* Packet buffer	*/
		)
{
	struct	rplentry *rplptr;
	struct	rpl_daoack *daokptr;

	// TODO For now...
	rplptr = &rpltab[0];

	if(!rplptr->daokwait) {
		return;
	}

	daokptr = (struct rpl_daoack *)pkt->net_icdata;

	if(daokptr->rpl_daoseq != rplptr->daoseq) {
		//#ifdef DEBUG_RPL
		kprintf("rpl_in_daok: Seq does not match. exp %d, act %d\n", rplptr->daoseq, daokptr->rpl_daoseq);
		//#endif
		return;
	}

	//#ifdef DEBUG_RPL
	kprintf("rpl_in_daok: seq matched\n");
	//#endif
	rplptr->daok = TRUE;
	rplptr->daokwait = FALSE;
}

/*------------------------------------------------------------------------
 * rpl_parents_cb  -  RPL Parents callback called by ND registration
 *------------------------------------------------------------------------
 */
void	rpl_parents_cb (
		int32	arg1,
		int32	arg2
		)
{
	rpl_parents(arg1);
}

/*------------------------------------------------------------------------
 * rpl_parents  -  Choose our RPl parents from neighbors (intr. off)
 *------------------------------------------------------------------------
 */
int32	rpl_parents (
		int32	rplindex	/* Index in RPL table		*/
		)
{
	struct	rplentry *rplptr;	/* RPL entry			*/
	struct	ip_fwentry *ipfwptr;	/* IP forwarding table entry	*/
	int32	sorted[MAX_RPL_NEIGHBORS];
					/* Sorted neighbor indexes	*/
	int32	i, j, t, n;		/* Index variables		*/
	int32	min;			/* Minimum value		*/
	int32	a, b;			/* Temporary value		*/
	int32	rank;			/* Rank				*/
	byte	hwaddr[6];		/* Hardware address of neighbor	*/

	rplptr = &rpltab[rplindex];

	n = rplptr->nneighbors;

	/* Initially, mark all neighbors as not parents */
	for(i = 0; i < n; i++) {
		sorted[i] = i;
		rplptr->neighbors[i].parent = FALSE;
		memcpy(hwaddr, &rplptr->neighbors[i].lluip[8], 8);
		if(hwaddr[0] & 0x02) {
			hwaddr[0] &= ~0x02;
		}
		else {
			hwaddr[0] |= 0x02;
		}
		for(j = 0; j < NBRTAB_SIZE; j++) {
			if(!memcmp(hwaddr, radtab[0].nbrtab[j].hwaddr, 8)) {
				break;
			}
		}
		if(j >= NBRTAB_SIZE) {
			//Candidate parent not in neighbor table!
		}

		rplptr->neighbors[i].pathcost = rplptr->neighbors[i].rank +
					(uint16)(radtab[0].nbrtab[j].etx*128);
		a = rplptr->neighbors[i].pathcost;
		b = rplptr->neighbors[i].rank + rplptr->minhoprankinc;
		rplptr->neighbors[i].pathrank = a > b ? a : b;
	}

	/* Sort all the neighbors according to the path cost */
	for(i = 0; i < n; i++) {
		min = i;
		for(j = i + 1; j < n; j++) {
			if(rplptr->neighbors[sorted[min]].pathcost >
					rplptr->neighbors[sorted[j]].pathcost) {
				min = j;
			}
		}
		t = sorted[i];
		sorted[i] = sorted[min];
		sorted[min] = t;
	}

	/* Cap n to maximum parent set size */
	n = RPL_PARENT_SET_SIZE > n ? n : RPL_PARENT_SET_SIZE;

	/* Mark the first n sorted neighbors as parents */
	rplptr->nparents = n;
	for(i = 0; i < n; i++) {
		rplptr->neighbors[sorted[i]].parent = TRUE;
		rplptr->parents[i] = sorted[i];
		//nd_regaddr(rplptr->neighbors[sorted[i]].lluip, rplptr->iface);
	}

	/* First sorted neighbor is our preferred parent */
	rplptr->prefparent = sorted[0];
	rplptr->cur_min_path_cost = rplptr->neighbors[sorted[0]].pathcost;

	/* Now let's compute our rank ... */

	rank = rplptr->neighbors[sorted[0]].pathrank;

	for(i = 0; i < n; i++) {
		j = sorted[i];
		a = rplptr->minhoprankinc * (1 + (rplptr->neighbors[j].rank/rplptr->minhoprankinc));
		if(rank < a) {
			rank = a;
		}

		a = rplptr->neighbors[j].pathrank - rplptr->maxrankinc;
		if(rank < a) {
			rank = a;
		}
	}

	#ifdef DEBUG_RPL
	kprintf("rpl_parents: Our rank is: %d\n", rank);
	#endif

	rplptr->rank = rank;

	/* Change the IP forwarding table to route through preferred parent by default */

	ipfwptr = &ip_fwtab[rplptr->iface];

	ipfwptr->ipfw_iface = rplptr->iface;
	ipfwptr->ipfw_prefix.prefixlen = 0;
	ipfwptr->ipfw_onlink = 0;
	memcpy(ipfwptr->ipfw_nxthop, rplptr->neighbors[rplptr->prefparent].lluip, 16);

	//#ifdef DEBUG_RPL
	kprintf("rpl_parents: default route through: ");
	ip_printaddr(rplptr->neighbors[rplptr->prefparent].lluip);
	kprintf("\n");
	//#endif

	ipfwptr->ipfw_state = IPFW_STATE_USED;

	rplptr->daokwait = TRUE;
	rplptr->daoktime = clktimems + 3000;
	resume(rplptr->timerproc);

	/* Send DAO message to RPL DODAG root */
	rpl_send_dao(rplindex, -1);

	return OK;
}

/*------------------------------------------------------------------------
 * rpl_send_dis  -  Send a DIS message
 *------------------------------------------------------------------------
 */
int32	rpl_send_dis (
		int32	rplindex,	/* Index in RPL table	*/
		int32	iface		/* Interface index	*/
		)
{
	struct	rplentry *rplptr;	/* Pointer to RPl entry	*/
	struct	rpl_dis *dismsg;	/* DIS message		*/
	struct	rplopt_solinfo *siopt;	/* Sol. info. option	*/
	int32	msglen;			/* Length of message	*/
	intmask	mask;			/* Interrupt mask	*/

	msglen = sizeof(struct rpl_dis) + sizeof(struct rplopt_solinfo);

	dismsg = (struct rpl_dis *)getmem(msglen);
	if((int32)dismsg == SYSERR) {
		return SYSERR;
	}

	siopt = (struct rplopt_solinfo *)dismsg->rpl_opts;

	memset(dismsg, 0, msglen);

	siopt->rplopt_type = RPLOPT_TYPE_SI;
	siopt->rplopt_len = sizeof(struct rplopt_solinfo) - 2;

	if(rplindex == -1) {
		if(iface < 0 || iface >= NIFACES) {
			return SYSERR;
		}

		#ifdef DEBUG_RPL
		kprintf("rpl_send_dis: Sending DIS to all-rpl-nodes-mc\n");
		#endif
		icmp_send(ICMP_TYPE_RPL, ICMP_CODE_RPL_DIS, ip_unspec, ip_allrplnodesmc,
				dismsg, msglen, iface);
		return OK;
	}

	if(rplindex < 0 || rplindex >= NRPL) {
		return SYSERR;
	}

	mask = disable();

	rplptr = &rpltab[rplindex];

	if(rplptr->state == RPL_STATE_FREE) {
		restore(mask);
		return SYSERR;
	}

	siopt->rplopt_insid = rplptr->rplinsid;
	siopt->rplopt_d = 1;
	siopt->rplopt_i = 1;
	siopt->rplopt_v = 1;
	memcpy(siopt->rplopt_dodagid, rplptr->dodagid, 16);
	siopt->rplopt_vers = rplptr->vers;

	icmp_send(ICMP_TYPE_RPL, ICMP_CODE_RPL_DIS, ip_unspec, ip_allrplnodesmc,
			dismsg, msglen, rplptr->iface);

	restore(mask);

	return OK;
}

/*------------------------------------------------------------------------
 * rpl_send_dio  -  Send a DIO message (intr. off)
 *------------------------------------------------------------------------
 */
int32	rpl_send_dio (
		int32	rplindex,	/* Index in RPL table		*/
		byte	ipdst[]		/* Destination IP address	*/
		)
{
	struct	rplentry *rplptr;	/* RPL table entry		*/
	struct	rpl_dio *diomsg;	/* DIO message			*/
	struct	rplopt_pi *piopt;	/* Prefix info. option		*/
	struct	rplopt_dodagconfig *dcopt;
					/* DODAG Config option		*/
	int32	msglen;			/* Total message length		*/
	byte	*rplopt;		/* Pointer to RPL option	*/
	int32	i;			/* Index variable		*/

	rplptr = &rpltab[rplindex];

	if(rplptr->state == RPL_STATE_FREE) {
		return SYSERR;
	}

	if(!rplptr->daok) {
		return SYSERR;
	}

	/* Compute the message length */
	msglen = sizeof(struct rpl_dio) +
		 sizeof(struct rplopt_dodagconfig) +
		 rplptr->nprefixes * sizeof(struct rplopt_pi);

	/* Allocate enough memory for the message */
	diomsg = (struct rpl_dio *)getmem(msglen);
	if((int32)diomsg == SYSERR) {
		return SYSERR;
	}

	/* Zero-out the bytes in the message */
	memset(diomsg, 0, msglen);

	diomsg->rpl_insid = rplptr->rplinsid;
	diomsg->rpl_vers = rplptr->vers;
	diomsg->rpl_rank = htons(rplptr->rank);
	diomsg->rpl_prf = rplptr->prf;
	diomsg->rpl_mop = rplptr->mop;
	diomsg->rpl_g = 1;
	diomsg->rpl_dtsn = rplptr->dtsn;
	memcpy(diomsg->rpl_dodagid, rplptr->dodagid, 16);

	dcopt = (struct rplopt_dodagconfig *)diomsg->rpl_opts;
	dcopt->rplopt_type = RPLOPT_TYPE_DC;
	dcopt->rplopt_len = sizeof(struct rplopt_dodagconfig) - 2;
	dcopt->rplopt_pcs = rplptr->pcs;
	dcopt->rplopt_a = 0;
	dcopt->rplopt_diointdbl = rplptr->diointdbl;
	dcopt->rplopt_diointmin = rplptr->diointmin;
	dcopt->rplopt_dioredun = rplptr->dioredun;
	dcopt->rplopt_maxrankinc = htons(rplptr->maxrankinc);
	dcopt->rplopt_minhoprankinc = htons(rplptr->minhoprankinc);
	dcopt->rplopt_ocp = rplptr->ocp;
	dcopt->rplopt_deflife = rplptr->deflife;
	dcopt->rplopt_lifeunit = htons(rplptr->lifeunit);

	rplopt = (byte *)(dcopt + 1);
	for(i = 0; i < rplptr->nprefixes; i++) {
		piopt = (struct rplopt_pi *)rplopt;
		piopt->rplopt_type = RPLOPT_TYPE_PI;
		piopt->rplopt_len = sizeof(struct rplopt_pi) - 2;
		piopt->rplopt_preflen = 64;
		piopt->rplopt_r = 1;
		piopt->rplopt_a = 1;
		piopt->rplopt_l = 0;
		piopt->rplopt_validlife = htonl(rplptr->prefixes[i].validlife);
		piopt->rplopt_preflife = htonl(rplptr->prefixes[i].preflife);
		memcpy(piopt->rplopt_prefix, rplptr->prefixes[i].prefix, 16);

		rplopt = (byte *)(piopt + 1);
	}

	#ifdef DEBUG_RPL
	kprintf("rpl_send_dio: sending %d bytes\n", msglen);
	#endif

	/* Send the message */
	icmp_send(ICMP_TYPE_RPL,
		  ICMP_CODE_RPL_DIO,
		  ip_unspec,
		  ipdst,
		  diomsg,
		  msglen,
		  rplptr->iface);

	return OK;
}

/*------------------------------------------------------------------------
 * rpl_send_dao  -  Send a DAO message to the root (intr. off)
 *------------------------------------------------------------------------
 */
int32	rpl_send_dao (
		int32	rplindex,	/* RPL table index	*/
		int32	pindex		/* Parent index		*/
		)
{
	struct	rplentry *rplptr;	/* RPL entry		*/
	struct	rpl_dao *daomsg;	/* DAO message		*/
	struct	rplopt_tgt *tgtopt;	/* Target option	*/
	struct	rplopt_transitinfo *tiopt;
					/* Transit Info. option */
	struct	netiface *ifptr;	/* Network interface	*/
	byte	*padopt;		/* Pad option		*/
	int32	msglen;			/* MEssage length	*/
	byte	pc;			/* Path Control		*/
	int32	i, n;			/* Index variables	*/
	static	int seq = 1;

	//#ifdef DEBUG_RPL
	kprintf("rpl_send_dao: ...\n");
	//#endif

	rplptr = &rpltab[rplindex];

	if(rplptr->state == RPL_STATE_FREE) {
		#ifdef DEBUG_RPL
		kprintf("rpl_send_dao: invalid rpl state\n");
		#endif
		return SYSERR;
	}

	ifptr = &iftab[rplptr->iface];

	if(pindex >= rplptr->nneighbors) {
		#ifdef DEBUG_RPL
		kprintf("rpl_send_dao: pindex %d, nneighbors %d\n", pindex, rplptr->nneighbors);
		#endif
		return SYSERR;
	}

	if((pindex != -1) && (rplptr->neighbors[pindex].parent == FALSE)) {
		#ifdef DEBUG_RPL
		kprintf("rpl_send_dao: neighbor is not a parent\n");
		#endif
		return SYSERR;
	}

	if(pindex == -1) { /* Send all parents' information */
		i = 0;
		n = rplptr->nparents;
	}
	else {	/* Send only one parent's information */
		i = pindex;
		n = pindex;
	}

	#ifdef DEBUG_RPL
	kprintf("rpl_send_dao: i = %d, n = %d\n", i, n);
	#endif

	msglen = sizeof(struct rpl_dao) +
		 sizeof(struct rplopt_tgt) +
		 (n-i+1)*(2 + sizeof(struct rplopt_transitinfo));

	daomsg = (struct rpl_dao *)getmem(msglen);
	if((int32)daomsg == SYSERR) {
		return SYSERR;
	}

	memset(daomsg, 0, msglen);

	daomsg->rpl_insid = rplptr->rplinsid;
	daomsg->rpl_d = 1;
	daomsg->rpl_k = 1;
	daomsg->rpl_daoseq = seq;
	memcpy(daomsg->rpl_dodagid, rplptr->dodagid, 16);

	tgtopt = (struct rplopt_tgt *)daomsg->rpl_opts;
	tgtopt->rplopt_type = RPLOPT_TYPE_TG;
	tgtopt->rplopt_len = sizeof(struct rplopt_tgt) - 2;
	tgtopt->rplopt_preflen = 128;
	memcpy(tgtopt->rplopt_tgtprefix, ifptr->if_ipucast[1].ipaddr, 16);
	#ifdef DEBUG_RPL
	kprintf("rpl_send_dao: target: "); ip_printaddr(tgtopt->rplopt_tgtprefix);
	kprintf("\n");
	#endif

	tiopt = (struct rplopt_transitinfo *)(tgtopt + 1);

	pc = 0xc0;
	for(; i < n; i++) {
		/*
		if(rplptr->neighbors[i].parent == FALSE) {
			continue;
		}
		*/

		tiopt->rplopt_type = RPLOPT_TYPE_TI;
		tiopt->rplopt_len = sizeof(struct rplopt_transitinfo) - 2;
		tiopt->rplopt_e = 0;
		tiopt->rplopt_pc = pc;
		tiopt->rplopt_pathseq = 1;
		tiopt->rplopt_pathlife = 100;
		memcpy(tiopt->rplopt_parent, rplptr->neighbors[i].gip, 16);
		#ifdef DEBUG_RPL
		kprintf("rpl_send_dao: parent: ");
		ip_printaddr(tiopt->rplopt_parent);
		kprintf("\n");
		#endif

		padopt = (byte *)(tiopt + 1);
		*padopt = RPLOPT_TYPE_PN;
		*(padopt + 1) = 0;

		pc >>= 2;
		if((pc & rplptr->pcmask) == 0) {
			break;
		}

		tiopt = (struct rplopt_transitinfo *)(padopt + 2);
	}

	rplptr->daoktime = clktimems + 1000;
	rplptr->daoseq = seq++;
	rplptr->daokwait = TRUE;

	resume(rplptr->timerproc);

	//#ifdef DEBUG_RPL
	kprintf("rpl_send_dao: sending..%d\n", msglen);
	//#endif
	icmp_send(ICMP_TYPE_RPL,
		  ICMP_CODE_RPL_DAO,
		  ip_unspec,
		  rplptr->dodagid,
		  daomsg,
		  msglen,
		  rplptr->iface);

	freemem((char *)daomsg, msglen);

	return OK;
}

/*------------------------------------------------------------------------
 * rpl_timer  -  RPL Timer process
 *------------------------------------------------------------------------
 */
process	rpl_timer (void) {

	struct	rplentry *rplptr;
	intmask	mask;

	rplptr = &rpltab[0];

	mask = disable();

	while(rplptr->state == RPL_STATE_FREE) {
		restore(mask);
		rpl_send_dis(-1, 1);
		sleep(1);
	}

	while(1) {

		mask = disable();

		if(rplptr->daokwait) {
			if(clktimems >= rplptr->daoktime) {
				rpl_send_dao(0, -1);
				rplptr->daokwait = TRUE;
				rplptr->daoktime = clktimems + 3000;
			}
			restore(mask);
			sleepms(rplptr->daoktime-clktimems);
			continue;
		}

		suspend(getpid());
	}

	return OK;
}
