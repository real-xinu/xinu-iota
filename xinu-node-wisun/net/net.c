/* net.c - net_init, netin */

#include <xinu.h>

bpid32	netbufpool;
int32	beserver;
struct	network NetData;
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

        /* Initialize the network data structure */
	memset((char *)&NetData, NULLCH, sizeof(struct network));

	control(ETHER0, ETH_CTRL_GET_MAC, (int32)NetData.ethucast, 0);

	memset((char *)NetData.ethbcast, 0xFF, ETH_ADDR_LEN);



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
/*
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
*/
	/* Create the ND timer process */

	resume(create(nd_timer, NETSTK, NETPRIO+10, "nd_timer", 0, NULL));

	/* Create a netin process for each interface */

	for(iface = 0; iface < NIFACES; iface++) {

		if(if_tab[iface].if_state == IF_DOWN) {
			continue;
		}

		if(if_tab[iface].if_type == IF_TYPE_RADIO) {
			sprintf(procname, "netin_%d", iface);
			//resume(create(netin, NETSTK, NETPRIO, procname, 1, iface));
		}
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
	int32	count;
	int32	i;

	if((iface < 0) || (iface > NIFACES)) {
		return SYSERR;
	}

	ifptr = &if_tab[iface];

	while(TRUE) {
		/*
		wait(ifptr->isem);

		pkt = ifptr->pktbuf[ifptr->head];
		ifptr->head++;
		if(ifptr->head >= 10) {
			ifptr->head = 0;
		}
		*/

		pkt = (struct netpacket *)getbuf(netbufpool);
		if(pkt == SYSERR) {
			panic("netin: Cannot allocate memory for packets");
		}

		count = read(ifptr->if_dev, (char *)pkt, PACKLEN);
		if(count == SYSERR) {
			panic("netin: Cannot read from device");
		}

		//kprintf("IN: "); pdump(pkt);

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

/*------------------------------------------------------------------------
 * rawin  -  Raw network input process to read from ethernet device
 *------------------------------------------------------------------------
 */
process rawin(void) {

	struct	etherPkt *pkt;
	//struct netpacket *pkt;
	int32	count;
	int i;
        //control(ETHER0, ETH_CTRL_PROMISC_ENABLE, 0,0);
	while(TRUE) {


		pkt = (struct etherPkt *)getbuf(netbufpool);
		//pkt = (struct netpacket *)getbuf(netbufpool);
		if(pkt == SYSERR) {
			panic("rawin: cannot allocate memory");
		}

		count = read(ETHER0, (char *)pkt, PACKLEN);
		//kprintf("count:%d\n", count);
		if(count == SYSERR) {
			panic("rawin: cannot read from ethernet");
		}
                
		switch(ntohs(pkt->type)) {
                //switch(ntohs(pkt->net_ethtype)){

		case 0x0000:
			if(semcount(radtab[0].isem) >= radtab[0].rxRingSize) {
				freebuf((char *)pkt);
				continue;
			}

			memcpy(((struct rad_rx_desc *)radtab[0].rxRing)[radtab[0].rxTail].buffer, pkt, count);
			radtab[0].rxTail = (radtab[0].rxTail + 1) % radtab[0].rxRingSize;

			freebuf((char *)pkt);
			signal(radtab[0].isem);
			break;

		case ETH_TYPE_A:
			amsg_handler(pkt);
			break;
		case ETH_TYPE_B:
			process_typb(pkt);
			break;

		default:
			//kprintf("rawin: unknown ethernet type: %02x\n", ntohs(pkt->type));
			freebuf((char *)pkt);
			break;
		}
	}

	return OK;
}
