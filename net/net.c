/* net.c - netin */

#include <xinu.h>
extern	process	icmp_echoproc(void);
bpid32	netbufpool;

/*------------------------------------------------------------------------
 * net_init  -  Initialize the network
 *------------------------------------------------------------------------
 */
void	net_init ()
{
	int32	iface;		/* Interface number	*/
	char	netstr[20];	/* Process name		*/

	/* Initialize the network interfaces */

	netiface_init();

	/* Create network buffer pool */

	netbufpool = mkbufpool(PACKLEN, 20);
	if((int32)netbufpool == SYSERR) {
		panic("Cannot create network buffer pool\n");
	}

	ipoqueue.iqsem = semcreate(0);
	if(ipoqueue.iqsem == SYSERR) {
		panic("Cannot create IP output queue semaphore\n");
	}
	ipoqueue.iqhead = 0;
	ipoqueue.iqtail = 0;

	/* Create a netin process for each interface */

	for(iface = 0; iface < NIFACES; iface++) {

		sprintf(netstr, "netin(iface = %d)", iface);
		resume(create(netin, NETSTK, NETPRIO, netstr, 1, iface));
	}

	/* Create the IP output process */

	resume(create(ipout, NETSTK, NETPRIO, "ipout", 0, NULL));

	resume(create(icmp_echoproc, 4096, NETPRIO, "icmp_echoproc", 0, NULL));

	return;
}

/*------------------------------------------------------------------------
 * netin  -  Process that reads incoming packets
 *------------------------------------------------------------------------
 */
process	netin (
	int32	iface
	)
{
	struct	netpacket *pkt;		/* Packet pointer	*/
	struct	if_entry  *ifptr;	/* Interface entry ptr	*/
	int32	retval;

	if( (iface < 0) || (iface >= NIFACES) ) {
		return SYSERR;
	}

	ifptr = &if_tab[iface];

	while(TRUE) {

		pkt = (struct netpacket *)getbuf(netbufpool);
		retval = read(ETHER0, (char *)pkt, 1576);
		pkt->net_iface = iface;
		ip_in(pkt);
	}

	return OK;
}
