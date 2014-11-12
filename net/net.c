/* net.c - net_init, netin */

#include <xinu.h>

bpid32	netbufpool;

/*------------------------------------------------------------------------
 * net_init  -  Initialize the network
 *------------------------------------------------------------------------
 */
void	net_init (void)
{
	char	procname[50];	/* Netin process name	*/
	int32	iface;		/* Interface number	*/

	/* Initialize the interfaces */

	netiface_init();

	/* Create the network buffer pool */

	netbufpool = mkbufpool(PACKLEN, 20);
	if((int32)netbufpool == SYSERR) {
		panic("Cannot create network buffer pool\n");
	}

	/* Create a netin process for each interface */

	for(iface = 0; iface < NIFACES; iface++) {

		if(if_tab[iface].if_state == IF_DOWN) {
			continue;
		}

		sprintf(procname, "netin_%d", iface);
		resume(create(netin, NETSTK, NETPRIO, procname, 1, iface));
	}
}

/*------------------------------------------------------------------------
 * netin  -  Network input process
 *------------------------------------------------------------------------
 */
process	netin (
	int32	iface
	)
{
	struct	ifentry *ifptr;	/* Pointer to interface	*/
	struct	netpacket *pkt;	/* Pointer to packet	*/

	if((iface < 0) || (iface > NIFACES)) {
		return SYSERR;
	}

	ifptr = &if_tab[iface];

	while(TRUE) {

		pkt = (struct netpacket *)getbuf(netbufpool);
		if((int32)pkt == SYSERR) {
			panic("netin cannot get buffer for packet\n");
		}

		read(RADIO, (char *)pkt, PACKLEN);

		if((pkt->net_ipvtch&0xf0) == 0x60) {
			kprintf("netin: incoming ipv6 packet\n");
			pkt->net_iface = iface;
			ip_in(pkt);
			//freebuf((char *)pkt);
		}
		else {
			freebuf((char *)pkt);
		}
	}

	return OK;
}
