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
	struct netpacket *pkt;		/* Pointer to new packet	*/
	byte		*lip;		/* Local IP address to use	*/
	byte		*rip;		/* Remote IP address (reply)	*/
	int32		len;		/* Length of the TCP data	*/

	/* Did we already send a RESET? */

	if (oldpkt->net_tcpcode & TCPF_RST)
		return SYSERR;

	/* Allocate a buffer */

	pkt = (struct netpacket *)getbuf(netbufpool);

	if ((int32)pkt == SYSERR) {
		return SYSERR;
	}

	/* Compute length of TCP data (needed for ACK) */

	len = oldpkt->net_iplen - TCP_HLEN(pkt);

	/* Obtain remote IP address */

	rip = oldpkt->net_ipsrc;
	lip = oldpkt->net_ipdst;

	/* Create TCP packet in pkt */

	/* Fill in IP header */

	pkt->net_ipvtch = 0x60;
	pkt->net_iptclflh = 0;
	pkt->net_ipfll = 0;
	pkt->net_iplen = TCP_HDR_LEN;
	pkt->net_ipnh = IP_TCP;
	pkt->net_iphl = 0xff;
	memcpy(pkt->net_ipsrc, lip, 16);
	memcpy(pkt->net_ipdst, rip, 16);

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

	ip_send(pkt);
	return OK;
}
