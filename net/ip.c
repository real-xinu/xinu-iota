/* ip.c - ip_in */

#include <xinu.h>

byte	ipllprefix[16] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0,
			  0, 0, 0, 0, 0, 0, 0, 0};
byte	ipunspec[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

struct	iqentry ipoqueue;	/* Network output queue	*/

/*------------------------------------------------------------------------
 * ip_local  -  Process packets destined for this node
 *------------------------------------------------------------------------
 */
void	ip_local (
	struct	netpacket *pkt
	)
{
	switch(pkt->net_ipnh) {

		case IP_ICMP:
			icmp_in(pkt);
			break;

		default:
			freebuf((char *)pkt);
			break;
	}
}

/*------------------------------------------------------------------------
 * ip_in  -  Process incoming IP datagrams
 *------------------------------------------------------------------------
 */
void	ip_in (
	struct	netpacket *pkt
	)
{
	int32	iface;		/* Incoming interface		*/
	struct	ifentry *ifptr;	/* Pointer to if_tab entry	*/
	uint32	longest;	/* Longest prefix match entry	*/
	int32	i;		/* For loop index		*/

	/* Get the incoming interface */

	iface = pkt->net_iface;
	if( (iface < 0) || (iface >= NIFACES) ) {
		freebuf((char *)pkt);
		return;
	}

	ifptr = &if_tab[iface];

	/* Check version number */

	if((pkt->net_ipvtch & 0xf0) != 0x60) {
		freebuf((char *)pkt);
		return;
	}

	ip_ntoh(pkt);

	/* Check the destination address */

	/*for(i = 0; i < 16; i++) {
		kprintf("%x ", pkt->net_ipdst[i]);
	}
	kprintf("\n");
	for(i = 0; i < 16; i++) {
		kprintf("%x ", ifptr->if_ipucast[0].ipaddr[i]);
	}
	kprintf("\n");*/
	longest = 0;
	for(i = 0 ; i < ifptr->if_nipucast; i++) {
		if(!memcmp(pkt->net_ipdst, ifptr->if_ipucast[i].ipaddr, 16)) {
			//kprintf("ip match %d\n", i);
			if(longest < ifptr->if_ipucast[i].preflen) {
				longest = ifptr->if_ipucast[i].preflen;
			}
		}
	}
	//kprintf("ip_in longest %d\n", longest);
	if(longest != -1) {
		ip_local(pkt);
		return;
	}

	freebuf((char *)pkt);
}

/*------------------------------------------------------------------------
 * ip_send  -  Send an IP packet
 *------------------------------------------------------------------------
 */
status	ip_send (
	struct	netpacket *pkt	/* Pointer to packet	*/
	)
{
	if(isipll(pkt->net_ipdst)) { /* Link local destination */

		memcpy(pkt->net_pandst, &pkt->net_ipdst[8], 8);
		memcpy(pkt->net_pansrc, if_tab[0].if_eui64, 8);

		ip_hton(pkt);

		write(ETHER0, (char *)pkt, ntohs(pkt->net_iplen) + 40 + 22);

		freebuf((char *)pkt);

		return OK;
	}
	else {
		freebuf((char *)pkt);

		return SYSERR;
	}
}

/*------------------------------------------------------------------------
 * ip_enqueue  -  Enqueue an IP packet for transmission
 *------------------------------------------------------------------------
 */
int32	ip_enqueue (
	struct	netpacket *pkt
	)
{
	intmask	mask;	/* Saved interrupt mask	*/

	mask = disable();

	if(semcount(ipoqueue.iqsem) >= IP_OQSIZ) {
		freebuf((char *)pkt);
		restore(mask);
		return SYSERR;
	}

	ipoqueue.iqbuf[ipoqueue.iqtail] = pkt;
	ipoqueue.iqtail += 1;
	if(ipoqueue.iqtail >= IP_OQSIZ) {
		ipoqueue.iqtail = 0;
	}

	restore(mask);

	signal(ipoqueue.iqsem);

	return OK;
}

/*------------------------------------------------------------------------
 * ipout  -  IP Output process
 *------------------------------------------------------------------------
 */
process	ipout ()
{
	intmask	mask;		/* Saved interrupt mask	*/
	struct	netpacket *pkt;	/* Pointer to packet	*/

	while(TRUE) {

		wait(ipoqueue.iqsem);

		mask = disable();

		pkt = ipoqueue.iqbuf[ipoqueue.iqhead];
		ipoqueue.iqhead += 1;
		if(ipoqueue.iqhead >= IP_OQSIZ) {
			ipoqueue.iqhead = 0;
		}

		restore(mask);

		ip_send(pkt);
	}

	return OK;
}

/*------------------------------------------------------------------------
 * ip_hton  -  Convert IP fields into network byte order
 *------------------------------------------------------------------------
 */
void	ip_hton (
	struct	netpacket *pkt
	)
{
	pkt->net_ipfll = htons(pkt->net_ipfll);
	pkt->net_iplen = htons(pkt->net_iplen);
}

/*------------------------------------------------------------------------
 * ip_ntoh  -  Convert IP fields into host byte order
 *------------------------------------------------------------------------
 */
void	ip_ntoh (
	struct	netpacket *pkt
	)
{
	pkt->net_ipfll = ntohs(pkt->net_ipfll);
	pkt->net_iplen = ntohs(pkt->net_iplen);
}
