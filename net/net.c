/* net.c - net_init, netin, rawin, freebuf_np */

#include <xinu.h>

/* Buffer pool for network packets */
bpid32	netbufpool;

/*------------------------------------------------------------------------
 * net_init  -  Initialize the network
 *------------------------------------------------------------------------
 */
void	net_init(void) {

	char	procname[20];
	int32	iface;

	/* Initialize all the network interfaces */
	netiface_init();

	/* Initialize ND related data structures */
	nd_init();

	/* Create a buffer pool of network packets */
	netbufpool = mkbufpool(PACKLEN, 50);

	/* Create and start the rawin process */
	resume(create(rawin, 8192, NETPRIO, "rawin", 0, NULL));

	/* Create a netin process for each interface */
	for(iface = 0; iface < NIFACES; iface++) {

		sprintf(procname, "netiface-%d", iface);
		resume(create(netin, 8192, NETPRIO, procname, 1, iface));
	}

}

/*------------------------------------------------------------------------
 * netin  -  Handle packets coming in on the network interface
 *------------------------------------------------------------------------
 */
process	netin(
		int32	iface	/* Index of network interface	*/
	     )
{
	struct	netiface *ifptr;
	struct	netpacket_e *epkt;
	struct	netpacket_r *rpkt;
	void	*pkt;
	intmask	mask;

	kprintf("netin for iface = %d\n", iface);

	ifptr = &iftab[iface];

	while(TRUE) {

		mask = disable();

		wait(ifptr->if_iqsem);
		pkt = ifptr->if_inputq[ifptr->if_iqhead];
		ifptr->if_iqhead++;
		if(ifptr->if_iqhead >= IFQSIZ) {
			ifptr->if_iqhead = 0;
		}

		restore(mask);

		if(ifptr->if_type == IF_TYPE_ETH) {
			epkt = (struct netpacket_e *)pkt;

			eth_ntoh(epkt);

			switch(epkt->net_ethtype) {

			case ETH_TYPE_A:
				amsg_handler(epkt);
				freebuf((char *)epkt);
				break;

			case ETH_IPV6:
				ip_in((struct netpacket *)epkt->net_ethdata);
				freebuf((char *)epkt);
				break;

			default:
				freebuf((char *)epkt);
				break;
			}
		}
		else if(ifptr->if_type == IF_TYPE_RAD) {
			rpkt = (struct netpacket_r *)pkt;
			ip_in((struct netpacket *)rpkt->net_raddata);
			freebuf((char *)rpkt);
		}
		else {
			freebuf((char *)pkt);
		}

		//freebuf((char *)pkt);
	}

	return OK;
}

/*------------------------------------------------------------------------
 * rawin  -  Handle packets from the real ethernet device
 *------------------------------------------------------------------------
 */
process	rawin(void) {

	struct	netpacket_e *epkt;
	struct	netpacket_r *rpkt;
	struct	netpacket_r *ackpkt;
	byte	ackbuf[14+24+30];
	int32	retval;
	intmask	mask;
	int32	rv;
	int32	free, i;

	while(TRUE) {

		epkt = (struct netpacket_e *)getbuf(netbufpool);

		retval = read(ETHER0, (char *)epkt, sizeof(struct netpacket_e));
		if(retval == SYSERR) {
			panic("rawin: cannot read from the ethernet device");
		}

		switch(ntohs(epkt->net_ethtype)) {

		case ETH_TYPE_B:

                        if (info.xonoff == 0)
			{
			  freebuf((char *)epkt);
			  break;
			}

			#ifdef DEBUG_RAWIN
			kprintf("rawin: incoming TYPE_B\n");
			#endif

			rv = srbit(epkt->net_ethdst, info.nodeid, 1);
			if(rv <= 0) {
				#ifdef DEBUG_RAWIN
				kprintf("rawin: srbit rv = %d from a non-neighbor:", rv);
				int32	i;
				for(i = 0 ; i < 6; i++) {
					kprintf("%02x ", epkt->net_ethsrc[i]);
				}
				kprintf("\n");
				#endif
				freebuf((char *)epkt);
				break;
			}

			#ifdef DEBUG_RAWIN
			kprintf("rawin: srbit rv = %d\n", rv);
			#endif
			rpkt = (struct netpacket_r *)epkt;

			if(memcmp(rpkt->net_raddst, iftab[1].if_hwucast, 8) &&
			   memcmp(rpkt->net_raddst, iftab[1].if_hwbcast, 8)) {
				#ifdef DEBUG_RAWIN
				kprintf("rawin: radpkt not for us");
				int32	i;
				for(i = 0; i < 8; i++) {
					kprintf("%02x ", rpkt->net_raddst[i]);
				}
				kprintf("\n");
				#endif
				freebuf((char *)rpkt);
				break;
			}

			switch(rpkt->net_radfc & RAD_FC_FT) {

			case RAD_FC_FT_DAT:

				free = -1;
				for(i = 0; i < NBRTAB_SIZE; i++) {
					if(radtab[0].nbrtab[i].state == NBR_STATE_FREE) {
						free = (free == -1) ? i : free;
						continue;
					}
					if(!memcmp(radtab[0].nbrtab[i].hwaddr, rpkt->net_radsrc, 8)) {
						break;
					}
				}
				if(i >= NBRTAB_SIZE) {
					if(free == -1) {
						panic("Neighbor Table full!");
						freebuf((char *)rpkt);
						break;
					}
					memcpy(radtab[0].nbrtab[free].hwaddr, rpkt->net_radsrc, 8);
					radtab[0].nbrtab[free].lastseq = rpkt->net_radseq;
					radtab[0].nbrtab[free].txattempts = 0;
					radtab[0].nbrtab[free].ackrcvd = 0;
					radtab[0].nbrtab[free].etx = 0;
					radtab[0].nbrtab[free].nextcalc = 1;
					radtab[0].nbrtab[free].calctime = 0;
					radtab[0].nbrtab[free].state = NBR_STATE_USED;
					i = free;
				}
				else {
					if(rpkt->net_radseq <= radtab[0].nbrtab[i].lastseq) {
						kprintf("rawin: stale packet from %d, %d %d\n", rpkt->net_radsrc[7], rpkt->net_radseq, radtab[0].nbrtab[i].lastseq);
						//freebuf((char *)rpkt);
						//break;
					}
					radtab[0].nbrtab[i].lastseq = rpkt->net_radseq;
				}

				if(rpkt->net_radfc & RAD_FC_AR) {

					#ifdef DEBUG_RAWIN
					kprintf("rawin: incoming radio data\n");
					#endif
					ackpkt = (struct netpacket_r *)ackbuf;

					memcpy(ackpkt->net_ethsrc, iftab[0].if_hwucast, 6);
					memcpy(ackpkt->net_ethdst, info.mcastaddr, 6);
					ackpkt->net_ethtype = htons(ETH_TYPE_B);

					ackpkt->net_radfc = RAD_FC_FT_ACK |
							   RAD_FC_DAM3 |
							   RAD_FC_FV2 |
							   RAD_FC_SAM3;

					memcpy(ackpkt->net_radsrc, iftab[1].if_hwucast, 8);
					memcpy(ackpkt->net_raddst, rpkt->net_radsrc, 8);

					ackpkt->net_radseq = rpkt->net_radseq;

					#ifdef DEBUG_RAWIN
					kprintf("rawin: sending ack\n");
					#endif
					write(ETHER0, (char *)ackpkt, 14 + 24 + 30);
				}

				mask = disable();
				if(semcount(iftab[1].if_iqsem) >= IFQSIZ) {
					restore(mask);
					freebuf((char *)epkt);
					break;
				}

				((struct netpacket *)rpkt->net_raddata)->net_iface = 1;
				iftab[1].if_inputq[iftab[1].if_iqtail++] = epkt;
				if(iftab[1].if_iqtail >= IFQSIZ) {
					iftab[1].if_iqtail = 0;
				}

				restore(mask);

				signal(iftab[1].if_iqsem);
				break;

			case RAD_FC_FT_ACK:

				#ifdef DEBUG_RAWIN
				kprintf("rawin: incoming radio ack\n");
				#endif
				if(!radtab[0].rowaiting) {
					freebuf((char *)epkt);
					break;
				}

				#ifdef DEBUG_RAWIN
				kprintf("rawin: incoming ack..\n");
				kprintf("\t%d %d\n", radtab[0].rowaitseq, rpkt->net_radseq);
				#endif
				if(radtab[0].rowaitseq == rpkt->net_radseq && (!memcmp(radtab[0].rowaitaddr, rpkt->net_radsrc, 8))) {
					send(radtab[0].ro_process, OK);
				}
				else {
					kprintf("rawin: ACK DOES NOT MATCH!!!!!!!!!!\n");
				}

				freebuf((char *)epkt);
				break;

			default:
				freebuf((char *)epkt);
				break;
			}
			break;

		default:
			mask = disable();

			if(semcount(iftab[0].if_iqsem) >= IFQSIZ) {
				restore(mask);
				freebuf((char *)epkt);
				break;
			}

			((struct netpacket *)epkt->net_ethdata)->net_iface = 0;
			iftab[0].if_inputq[iftab[0].if_iqtail++] = epkt;
			if(iftab[0].if_iqtail >= IFQSIZ) {
				iftab[0].if_iqtail = 0;
			}

			restore(mask);

			signal(iftab[0].if_iqsem);
			break;
		}
	}

	return OK;
}

/*------------------------------------------------------------------------
 * eth_hton  -  Convert Ethernet header fields in network byte order
 *------------------------------------------------------------------------
 */
void	eth_hton (
		struct	netpacket_e *epkt
		)
{
	epkt->net_ethtype = htons(epkt->net_ethtype);
}

/*------------------------------------------------------------------------
 * eth_ntoh  -  Convert Ethernet header fields in host byte order
 *------------------------------------------------------------------------
*/
void	eth_ntoh (
		struct	netpacket_e *epkt
		)
{
	epkt->net_ethtype = ntohs(epkt->net_ethtype);
}
