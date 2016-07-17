/* tcpreset.c  -  tcpreset */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  tcpreset  -  Send a TCP RESET segment
 *------------------------------------------------------------------------
 */
int32	tcpreset(
	  struct netpacket *oldpkt	/* Ptr to old packet		*/
	)
{
	struct 	netpacket *pkt;		/* Pointer to new packet	*/
	byte	lip[16];		/* Local IP address to use	*/
	byte	rip[16];		/* Remote IP address (reply)	*/
	int32	len;			/* Length of the TCP data	*/

	/* Did we already send a RESET? */

	if (oldpkt->net_tcpcode & TCPF_RST)
		return SYSERR;

	/* Allocate a buffer */

	pkt = (struct netpacket *)getbuf(netbufpool);

	if ((int32)pkt == SYSERR) {
		return SYSERR;
	}

	/* Compute length of TCP data (needed for ACK) */

	len = oldpkt->net_iplen - IP_HDR_LEN - TCP_HLEN(pkt);

	/* Obtain remote IP address */

	memcpy(rip, oldpkt->net_ipsrc, 16);
	memcpy(lip, oldpkt->net_ipdst, 16);

	/* Create TCP packet in pkt */
	#if 0
	memcpy((char *)pkt->net_ethsrc, NetData.ethucast, ETH_ADDR_LEN);
	pkt->net_ethtype = 0x0800;	/* Type is IP */
	#endif
	/* Fill in IP header */

	pkt->net_ipvtch = 0x60;		/* IP version and hdr length	*/
	pkt->net_iptclflh = 0;		/* IP traf class and flow label	*/
	pkt->net_iplen = TCP_HLEN(pkt);
					/* total datagram length	*/
	pkt->net_iphl = 0xff;		/* IP time-to-live		*/
	pkt->net_ipnh = IP_TCP;		/* datagram carries TCP		*/
	memcpy(pkt->net_ipsrc, lip, 16);/* IP source address		*/
	memcpy(pkt->net_ipdst, rip, 16);/* IP destination address	*/

	/* Fill in TCP header */

	pkt->net_tcpsport = oldpkt->net_tcpdport;
	pkt->net_tcpdport = oldpkt->net_tcpsport;
	if (oldpkt->net_tcpcode & TCPF_ACK) {
		pkt->net_tcpseq = oldpkt->net_tcpack;
	} else {
		pkt->net_tcpseq = 0;
	}
	pkt->net_tcpack = oldpkt->net_tcpseq + len;
	if (oldpkt->net_tcpcode & (TCPF_SYN|TCPF_FIN)) {
		pkt->net_tcpack++;
	}
	pkt->net_tcpcode = TCP_HDR_LEN << 10;
	pkt->net_tcpcode |= TCPF_ACK | TCPF_RST;
	pkt->net_tcpwindow = 0;
	pkt->net_tcpcksum = 0;
	pkt->net_tcpurgptr = 0;

	/* Call ip_send to send the datagram */

	//ip_enqueue(pkt);
	return OK;
}
