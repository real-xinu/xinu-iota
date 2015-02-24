/* tcpalloc.c  -  tcpalloc */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  tcpalloc  -  allocate a buffer for an IP datagram carrying TCP
 *------------------------------------------------------------------------
 */
struct netpacket *tcpalloc(
	  struct tcb	*tcbptr,	/* Ptr to a TCB			*/
	  int32		len		/* Length of the TCP segment	*/
	)
{
	struct netpacket *pkt;		/* Ptr to packet for new struct	*/

	/* Allocate a buffer for a TCB */

	pkt = (struct netpacket *) getbuf(netbufpool);

	/* Set Ethernet type */

	/* Fill in Ethernet header */
	#if 0
	memcpy((char *)pkt->net_ethsrc, NetData.ethucast, ETH_ADDR_LEN);
	pkt->net_ethtype = 0x0800;	/* Type is IP */
	#endif
	/* Fill in IP header */

	pkt->net_ipvtch = 0x60;		/* IP version and traffic class	*/
	pkt->net_iptclflh = 0;		/* IP traffic class and flow	*/
	pkt->net_iplen= TCP_HDR_LEN + len;
					/* Total datagram length	*/
	pkt->net_iphl = 0xff;		/* IP time-to-live		*/
	pkt->net_ipnh = IP_TCP;		/* datagram carries TCP		*/
	memcpy(pkt->net_ipsrc, tcbptr->tcb_lip, 16);
					/* IP source address		*/
	memcpy(pkt->net_ipdst, tcbptr->tcb_rip, 16);
					/* IP destination address	*/

	pkt->net_iface = tcbptr->tcb_iface;

	/* Set the TCP port fields in the segment */

	pkt->net_tcpsport = tcbptr->tcb_lport;
	pkt->net_tcpdport = tcbptr->tcb_rport;

	/* Set the code field (includes Hdr length */

	pkt->net_tcpcode = (TCP_HDR_LEN << 10);

	if (tcbptr->tcb_state > TCB_SYNSENT) {
		pkt->net_tcpcode |= TCPF_ACK;
		pkt->net_tcpack = tcbptr->tcb_rnext;
	}
	kprintf("tcpalloc: tcp ack %d\n", pkt->net_tcpack);
	/* Set the window advertisement */

	pkt->net_tcpwindow = tcbptr->tcb_rbsize - tcbptr->tcb_rblen;

	/* Silly Window Avoidance belongs here... */

	/* Initialize the checksum and urgent pointer */
	pkt->net_tcpcksum = 0;
	pkt->net_tcpurgptr = 0;

	/* Return entire packet to caller */

	return pkt;
}
