/* net.c - net_init, netin, rawin, freebuf_np */

#include <xinu.h>

/* Buffer pool for network packets */
bpid32	netbufpool;
struct network NetData;
/*------------------------------------------------------------------------
 * net_init  -  Initialize the network
 *------------------------------------------------------------------------
 */
void	net_init(void) {

	char	procname[20];
	int32	iface;
        /* Initialize the network data structure */
	memset((char *)&NetData, NULLCH, sizeof(struct network));
        
	control(ETHER0, ETH_CTRL_GET_MAC, (int32)NetData.ethucast, 0);

	memset((char *)NetData.ethbcast, 0xFF, ETH_ADDR_LEN);

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
		if(ifptr->if_iqhead >= 10) {
			ifptr->if_iqhead = 0;
		}

		restore(mask);

		if(ifptr->if_type == IF_TYPE_ETH) {
			epkt = (struct netpacket_e *)pkt;

			eth_ntoh(epkt);

			switch(epkt->net_ethtype) {

			case ETH_IPV6:
				//kprintf("Incoming IPv6 packet\n");
				ip_in((struct netpacket *)epkt->net_ethdata);
				freebuf((char *)epkt);
				break;

			case ETH_TYPE_A:
				amsg_handler(epkt);
				break;
			case ETH_TYPE_B:
				//process_typb(epkt);
				break;


			default:
				freebuf((char *)epkt);
				break;
			}
		}
		else if(ifptr->if_type == IF_TYPE_RAD) {
			kprintf("Incoming radio packet\n");
			rpkt = (struct netpacket_r *)pkt;
		}
		else {
			freebuf((char *)pkt);
		}

		freebuf((char *)pkt);
	}

	return OK;
}

/*------------------------------------------------------------------------
 * rawin  -  Handle packets from the real ethernet device
 *------------------------------------------------------------------------
 */
process	rawin(void) {

	struct	netpacket_e *epkt;
	struct	netpacket *pkt;
	struct	netiface *ifptr;
	int32	retval;
	intmask	mask;
	int32	count = 0;

	kprintf("rawin created\n");

	while(TRUE) {

		epkt = (struct netpacket_e *)getbuf(netbufpool);

		retval = read(ETHER0, epkt, sizeof(struct netpacket_e));
		if(retval == SYSERR) {
			panic("rawin: cannot read from the ethernet device");
		}

		//kprintf("incoming packet type: %02x\n", ntohs(epkt->net_ethtype));

		switch(ntohs(epkt->net_ethtype)) {

		case ETH_RAD:
			kprintf("rawin: radio packet: %d\n", ++count);
			ifptr = &iftab[1];
			//pkt->net_iface = 1;
			freebuf((char *)epkt);
			continue;
			break;

		default:
			ifptr = &iftab[0];
			pkt = (struct netpacket *)epkt->net_ethdata;
			pkt->net_iface = 0;
			break;
		}

		mask = disable();

		if(semcount(ifptr->if_iqsem) >= 10) {
			kprintf("Interface queue full\n");
			freebuf((char *)epkt);
			continue;
		}

		ifptr->if_inputq[ifptr->if_iqtail] = epkt;
		ifptr->if_iqtail++;
		if(ifptr->if_iqtail >= 10) {
			ifptr->if_iqtail = 0;
		}

		signal(ifptr->if_iqsem);
		restore(mask);
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
