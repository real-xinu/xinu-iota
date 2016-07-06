/* net.c - net_init, netin */

#include <xinu.h>

bpid32	netbufpool;
int32	beserver;
extern	process nd_timer(void);
/*------------------------------------------------------------------------
 * net_init  -  Initialize the network
 *------------------------------------------------------------------------
 */
void	net_init (void)
{
	char	procname[50];	/* Netin process name	*/
	int32	iface;		/* Interface number	*/
	char	buf[10] = {0};

	/* Initialize the interfaces */

	netiface_init();

	ip_init();

	udp_init();

	tcp_init();

	/* Create the network buffer pool */

	netbufpool = mkbufpool(PACKLEN, 40);
	if((int32)netbufpool == SYSERR) {
		panic("Cannot create network buffer pool\n");
	}

	while(TRUE) {
		kprintf("Enter the backend number of the server: ");
		memset(buf, 0, 10);
		read(CONSOLE, buf, 10);
		beserver = atoi(buf);
		if((beserver < 101) || (beserver > 196)) {
			kprintf("Invalid backend number, must be in range [101,196]\n");
			continue;
		}
		beserver -= 101;
		break;
	}

	/* Create the ND timer process */

	resume(create(nd_timer, NETSTK, NETPRIO+10, "nd_timer", 0, NULL));

	/* Create a netin process for each interface */

	for(iface = 0; iface < NIFACES; iface++) {

		if(if_tab[iface].if_state == IF_DOWN) {
			continue;
		}

		sprintf(procname, "netin_%d", iface);
		resume(create(netin, NETSTK, NETPRIO, procname, 1, iface));
	}

	resume(create(rawin, NETSTK, NETPRIO+1, "rawin", 0, NULL));
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
	int32	i;

	if((iface < 0) || (iface > NIFACES)) {
		return SYSERR;
	}

	ifptr = &if_tab[iface];

	while(TRUE) {

		wait(ifptr->isem);

		pkt = ifptr->pktbuf[ifptr->head];
		ifptr->head++;
		if(ifptr->head >= 10) {
			ifptr->head = 0;
		}

		/*
		pkt = (struct netpacket *)getbuf(netbufpool);
		if((int32)pkt == SYSERR) {
			panic("netin cannot get buffer for packet\n");
		}

		read(RADIO0, (char *)pkt, PACKLEN);
		*/

		kprintf("IN: "); pdump(pkt);

		if((pkt->net_ipvtch&0xf0) == 0x60) {
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
