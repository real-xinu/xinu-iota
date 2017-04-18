/* rpl_lbr.c - rpl_lbr_init */

#include <xinu.h>

#define	DEBUG_RPL	1

struct	rplnode rplnodes[NRPLNODES];

byte	global_ipprefix[] = {0x20, 0x01, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0};

/*------------------------------------------------------------------------
 * rpl_lbr_init  -  Initialize RPL LBR data structures
 *------------------------------------------------------------------------
 */
void	rpl_lbr_init (void) {

	struct	rplnode *nodeptr;
	struct	rplentry *rplptr;
	struct	netiface *ifptr;
	struct	ip_fwentry *ipfptr;
	intmask	mask;
	int32	i;

	mask = disable();

	for(i = 0; i < NRPLNODES; i++) {

		nodeptr = &rplnodes[i];

		memset(nodeptr, 0, sizeof(struct rplnode));

		nodeptr->state = RPLNODE_STATE_FREE;
	}

	/* For now, add a global IP address to interface */

	ifptr = &iftab[1];
	memcpy(ifptr->if_ipucast[1].ipaddr, global_ipprefix, 8);
	memcpy(&ifptr->if_ipucast[1].ipaddr[8], ifptr->if_hwucast, 8);
	if(ifptr->if_hwucast[0] & 0x02) {
		ifptr->if_ipucast[1].ipaddr[8] &= 0xfd;
	}
	else {
		ifptr->if_ipucast[1].ipaddr[8] |= 0x02;
	}
	ifptr->if_ipucast[1].prefixlen = 8;
	ifptr->if_nipucast++;

	/* Add an entry in IP forwarding table */

	ipfptr = &ip_fwtab[0];
	memcpy(ipfptr->ipfw_prefix.ipaddr, global_ipprefix, 16);
	ipfptr->ipfw_prefix.prefixlen = 8;
	ipfptr->ipfw_onlink = 0;
	ipfptr->ipfw_iface = 1;
	ipfptr->ipfw_state = 1;

	/* Add a RPL entry and make us root of the DODAG */

	rplptr = &rpltab[0];

	rplptr->iface = 1;
	rplptr->root = TRUE;
	rplptr->rplinsid = 1;
	memcpy(rplptr->dodagid, iftab[1].if_ipucast[1].ipaddr, 16);
	rplptr->vers = 1;
	rplptr->grounded = 0;
	rplptr->prf = 0;
	rplptr->mop = 1;
	rplptr->dtsn = 1;
	rplptr->pcs = 7;
	rplptr->pcmask = 0xff;
	rplptr->maxrankinc = 0;
	rplptr->minhoprankinc = 256;
	rplptr->ocp = RPL_OCP_MRHOF;
	rplptr->deflife = 1;
	rplptr->lifeunit = 60;
	rplptr->rank = 256;
	rplptr->nneighbors = 0;
	rplptr->nparents = 0;
	rplptr->prefparent = -1;
	rplptr->nroutes = 0;
	rplptr->nprefixes = 1;
	memcpy(rplptr->prefixes[0].prefix, iftab[1].if_ipucast[1].ipaddr, 16);
	rplptr->prefixes[0].prefixlen = 128;

	rplptr->state = RPL_STATE_USED;

	memcpy(rplnodes[0].ipprefix, rplptr->dodagid, 16);
	rplnodes[0].nparents = 0;
	rplnodes[0].rplindex = 0;
	rplnodes[0].state = RPLNODE_STATE_USED;

	restore(mask);
}

/*------------------------------------------------------------------------
 * rpl_in_dao  -  Handle incoming DAO message
 *------------------------------------------------------------------------
 */
void	rpl_in_dao (
		struct	netpacket *pkt	/* Packet buffer	*/
		)
{
	struct	rpl_dao *daomsg;	/* DAO message		*/
	struct	rplentry *rplptr;	/* RPL entry		*/
	struct	rplopt_tgt *tgtopt;	/* Target option	*/
	struct	rplopt_transitinfo *tiopt;
					/* Transit Info. option	*/
	struct	rpl_daoack daok;
	byte	*rplopt;		/* Generic RPL option	*/
	intmask	mask;			/* Interrupt mask	*/
	int32	pindex;			/* Parent index		*/
	int32	free = -1;		/* Unused entry index	*/
	int32	i, j, k;		/* Index variables	*/

	#ifdef DEBUG_RPL
	kprintf("rpl_in_dao: ... %d IP len\n", pkt->net_iplen);
	#endif

	daomsg = (struct rpl_dao *)pkt->net_icdata;

	mask = disable();

	/* Look for a matching RPL instance ... */

	for(i = 0; i < NRPL; i++) {

		rplptr = &rpltab[i];

		if(rplptr->state == RPL_STATE_FREE) {
			continue;
		}

		if(!rplptr->root) {
			continue;
		}

		if(rplptr->rplinsid != daomsg->rpl_insid) {
			continue;
		}

		if(daomsg->rpl_d && (memcmp(rplptr->dodagid, daomsg->rpl_dodagid, 16))) {
				continue;
		}

		break;
	}
	if(i >= NRPL) {
		restore(mask);
		return;
	}

	/* Parse the options in the message */

	rplopt = daomsg->rpl_opts;
	while(rplopt < (pkt->net_ipdata + pkt->net_iplen)) {

		switch(*rplopt) {

		/* PAD1 option ... */
		case RPLOPT_TYPE_P1:
			rplopt++;
			break;

		/* PADN option ... */
		case RPLOPT_TYPE_PN:
			rplopt += (*(rplopt+1)) + 2;
			break;

		/* Target option ... */
		case RPLOPT_TYPE_TG:
			tgtopt = (struct rplopt_tgt *)rplopt;
			#ifdef DEBUG_RPL
			kprintf("rpl_in_dao: target: ");
			ip_printaddr(tgtopt->rplopt_tgtprefix);
			kprintf("\n");
			#endif

			/* Look for the target in our list ... */

			for(i = 0; i < NRPLNODES; i++) {

				if(rplnodes[i].state == RPLNODE_STATE_FREE) {
					free = (free == -1) ? i : free;
					continue;
				}

				if(!memcmp(rplnodes[i].ipprefix, tgtopt->rplopt_tgtprefix, 16)) {
					break;
				}
			}
			if(i >= NRPLNODES) { /* Cannot find the target, add it if possible */
				if(free == -1) { /* No free slots, fail */
					restore(mask);
					return;
				}

				/* Add the target in our list */
				i = free;
				//TODO: Assumes the prefix length is 128, change this!
				memcpy(rplnodes[i].ipprefix, tgtopt->rplopt_tgtprefix, 16);
				rplnodes[i].ippreflen = tgtopt->rplopt_preflen;
				rplnodes[i].nparents = 0;
				rplnodes[i].state = RPLNODE_STATE_USED;
			}

			/* Look for the following Transit Information options ... */

			rplopt += tgtopt->rplopt_len + 2;
			while(rplopt < (pkt->net_ipdata + pkt->net_iplen)) {

				if((*rplopt) == RPLOPT_TYPE_P1) {
					rplopt++;
					continue;
				}

				if((*rplopt) == RPLOPT_TYPE_PN) {
					rplopt += 2 + (*(rplopt+1));
					continue;
				}

				if((*rplopt) != RPLOPT_TYPE_TI) {
					break;
				}

				tiopt = (struct rplopt_transitinfo *)rplopt;
				#ifdef DEBUG_RPL
				kprintf("rpl_in_dao: parent: ");
				ip_printaddr(tiopt->rplopt_parent);
				kprintf("\n");
				#endif

				for(j = 0; j < rplnodes[i].nparents; j++) {
					pindex = rplnodes[i].parents[j].nindex;
					if(rplnodes[pindex].state == RPLNODE_STATE_FREE) {
						continue;
					}

					if(!memcmp(rplnodes[pindex].ipprefix, tiopt->rplopt_parent, 16)) {
						if(tiopt->rplopt_pathseq <= rplnodes[i].parents[j].ps) {
							break;
						}
						rplnodes[i].parents[j].ps = tiopt->rplopt_pathseq;
						rplnodes[i].parents[j].pc = tiopt->rplopt_pc;
						if(tiopt->rplopt_pc > rplnodes[i].parents[rplnodes[i].prefparent].pc) {
							rplnodes[i].prefparent = i;
						}
						break;
					}
				}
				if(j >= rplnodes[i].nparents) {
					if(rplnodes[i].nparents >= RPL_PARENT_SET_SIZE) {
						rplopt += tiopt->rplopt_len + 2;
						break;
					}
					for(j = 0; j < NRPLNODES; j++) {
						if(rplnodes[j].state == RPLNODE_STATE_FREE) {
							continue;
						}
						if(!memcmp(rplnodes[j].ipprefix, tiopt->rplopt_parent, 16)) {
							#ifdef DEBUG_RPL
							kprintf("rpl_in_dao: parent is at index %d\n", j);
							#endif
							k = rplnodes[i].nparents;
							rplnodes[i].parents[k].nindex = j;
							rplnodes[i].parents[k].ps = tiopt->rplopt_pathseq;
							rplnodes[i].parents[k].pc = tiopt->rplopt_pc;
							//TODO rplnodes[i].parents[k].texpire =

							if(rplnodes[i].nparents == 0) {
								rplnodes[i].prefparent = j;
							}
							else if(tiopt->rplopt_pc > rplnodes[i].parents[rplnodes[i].prefparent].pc) {
								rplnodes[i].prefparent = j;
							}
							#ifdef DEBUG_RPL
							kprintf("\tprefered parent of %d is at index %d\n", i, rplnodes[i].prefparent);
							#endif
							rplnodes[i].nparents++;
							break;
						}
					}
					if(j >= NRPLNODES) {
						#ifdef DEBUG_RPL
						kprintf("rpl_in_dao: cannot find parent in the set of nodes!\n");
						#endif
					}
				}

				rplopt += tiopt->rplopt_len + 2;
			}
			break;

		default:
			rplopt += (*(rplopt+1)) + 2;
			break;
		}
	}

	daok.rpl_insid = daomsg->rpl_insid;
	daok.rpl_daoseq = daomsg->rpl_daoseq;
	daok.rpl_status = 0;

	#ifdef DEBUG_RPL
	kprintf("rpl_in_dao: Sending DAOK seq %d to: ", daok.rpl_daoseq);
	ip_printaddr(pkt->net_ipsrc); kprintf("\n");
	#endif
	icmp_send(ICMP_TYPE_RPL, ICMP_CODE_RPL_DAOK, ip_unspec, pkt->net_ipsrc, &daok, sizeof(daok), rplptr->iface);

	restore(mask);
}

/*------------------------------------------------------------------------
 * ip_send_rpl_lbr  -  Send an IP packet over the radio interface
 *------------------------------------------------------------------------
 */
int32	ip_send_rpl_lbr (
		struct	netpacket *pkt,		/* Packet buffer	*/
		struct	netpacket_r *rpkt	/* Radio packet buffer	*/
		)
{
	struct	netpacket *npkt;	/* IP packet pointer	*/
	struct	ip_ext_hdr *exptr;	/* IP extension header	*/
	struct	netiface *ifptr;	/* Network interface	*/
	int32	path[30];
	int32	curr;
	intmask	mask;
	int32	ncindex;
	int32	iplen;
	int32	i, j;

	mask = disable();

	for(i = 0; i < NRPLNODES; i++) {
		if(rplnodes[i].state == RPLNODE_STATE_FREE) {
			continue;
		}

		if(!memcmp(pkt->net_ipdst, rplnodes[i].ipprefix, 16)) {
			break;
		}
	}
	if(i >= NRPLNODES) {
		freebuf((char *)pkt);
		restore(mask);
		return SYSERR;
	}

	path[0] = i;
	curr = rplnodes[i].prefparent;
	i = 1;
	#ifdef DEBUG_RPL
	kprintf("node %d, parent %d\n", path[0], curr); 
	#endif
	while(curr != 0) {
		path[i++] = curr;
		curr = rplnodes[curr].prefparent;
		#ifdef DEBUG_RPL
		kprintf("curr: %d\n", curr);
		#endif
	}

	#ifdef DEBUG_RPL
	kprintf("Path: 0 -> ");
	for(j = i-1; j >= 0; j--) {
		kprintf("%d ", path[j]);
		if(j > 0) {
			kprintf("-> ");
		}
	}
	kprintf("\n");
	#endif

	ifptr = &iftab[pkt->net_iface];

	/*
	rpkt = (struct netpacket_r *)getbuf(netbufpool);
	if((int32)rpkt == SYSERR) {
		restore(mask);
		return SYSERR;
	}
	*/

	memcpy(rpkt->net_ethsrc, iftab[0].if_hwucast, 6);
	//memcpy(rpkt->net_ethdst, info.mcastaddr, 6);
	rpkt->net_ethtype = htons(ETH_TYPE_B);

	rpkt->net_radfc = (RAD_FC_FT_DAT |
			   RAD_FC_AR |
			   RAD_FC_DAM3 |
			   RAD_FC_FV2 |
			   RAD_FC_SAM3);

	rpkt->net_radseq = ifptr->if_seq++;

	memcpy(rpkt->net_radsrc, ifptr->if_hwucast, 8);

	iplen = ntohs(pkt->net_iplen);

	if(i == 1) {
		memcpy(rpkt->net_raddata, pkt, 40 + iplen);
		npkt = (struct netpacket *)rpkt->net_raddata;
	}
	else {
		npkt = (struct netpacket *)rpkt->net_raddata;
		memset(npkt, 0, 40);
		npkt->net_ipvtch = 0x60;
		npkt->net_ipnh = IP_EXT_RH;
		npkt->net_iphl = 0xff;
		memcpy(npkt->net_ipsrc, ifptr->if_ipucast[1].ipaddr, 16);
		memcpy(npkt->net_ipdst, rplnodes[path[--i]].ipprefix, 16);
		#ifdef DEBUG_RPL
		kprintf("IPdst is: "); ip_printaddr(npkt->net_ipdst); kprintf("\n");
		#endif

		exptr = (struct ip_ext_hdr *)npkt->net_ipdata;
		exptr->ipext_nh = IP_IP;
		exptr->ipext_len = (2 * i);
		exptr->ipext_rhrt = 0;
		exptr->ipext_rhsegs = i;

		j = 0;
		while(i > 0) {
			memcpy(exptr->ipext_rhaddrs[j++], rplnodes[path[--i]].ipprefix, 16);
			#ifdef DEBUG_RPL
			kprintf("entry at slot %d: ", j-1); ip_printaddr(exptr->ipext_rhaddrs[j-1]); kprintf("\n");
			#endif
		}

		#ifdef DEBUG_RPL
		kprintf("Copying original packet..%d\n", iplen);
		for(i = 0; i < 40; i++) {
			kprintf("%02x ", *((byte *)pkt + i));
		}
		kprintf("\n");
		#endif
		memcpy(exptr->ipext_rhaddrs[j], pkt, 40 + iplen);

		npkt->net_iplen = ((byte *)exptr->ipext_rhaddrs[j]-npkt->net_ipdata) + 40 + iplen;
		iplen = npkt->net_iplen;
		ip_hton(npkt);
	}

	#ifdef DEBUG_RPL
	kprintf("Finding neighbor..");
	#endif
	ncindex = nd_ncfind(npkt->net_ipdst);

	if(ncindex == SYSERR) {
		freebuf((char *)pkt);
		freebuf((char *)rpkt);
		restore(mask);
		return SYSERR;
	}

	#ifdef DEBUG_RPL
	kprintf("found neighbor\n");
	#endif

	memcpy(rpkt->net_raddst, nd_ncache[ncindex].nc_hwaddr, 8);

	#ifdef DEBUG_RPL
	for(i = 0; i < 14+24+40+8+40+20; i++) {
		kprintf("%02x ", *((byte *)rpkt + i));
	}
	kprintf("\n");
	#endif

	write(RADIO0, (char *)rpkt, 14 + 24 + 40 + iplen);

	freebuf((char *)pkt);
	freebuf((char *)rpkt);

	restore(mask);
	return OK;
}
