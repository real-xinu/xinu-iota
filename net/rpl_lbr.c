/* rpl_lbr.c - rpl_lbr_init */

#include <xinu.h>

#define	DEBUG_RPL	1

struct	rplnode rplnodes[NRPLNODES];

/*------------------------------------------------------------------------
 * rpl_lbr_init  -  Initialize RPL LBR data structures
 *------------------------------------------------------------------------
 */
void	rpl_lbr_init (void) {

	struct	rplnode *nodeptr;
	struct	rplentry *rplptr;
	intmask	mask;
	int32	i;

	mask = disable();

	for(i = 0; i < NRPLNODES; i++) {

		nodeptr = &rplnodes[i];

		memset(nodeptr, 0, sizeof(struct rplnode));

		nodeptr->state = RPLNODE_STATE_FREE;
	}

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
								rplnodes[i].prefparent = k;
							}
							else if(tiopt->rplopt_pc > rplnodes[i].parents[rplnodes[i].prefparent].pc) {
								rplnodes[i].prefparent = k;
							}
							rplnodes[i].nparents++;
							break;
						}
					}
					if(j >= NRPLNODES) {
						kprintf("rpl_in_dao: cannot find parent in the set of nodes!\n");
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

	restore(mask);
}
