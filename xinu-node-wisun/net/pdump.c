/* psump.c - pdump, pdumph */

#include <xinu.h>

/*------------------------------------------------------------------------
 * pdump  -  Dump packet contents (packet in network byte order)
 *------------------------------------------------------------------------
 */
void	pdump (
	struct	netpacket *pkt
	)
{
	struct	nd_nbrsol *nbrsol;
	struct	nd_nbradv *nbradv;
	int32	i;

	if(pkt == NULL) {
		return;
	}

	kprintf("Eth (");
	for(i = 0; i < 5; i++) {
		kprintf("%02x:", pkt->net_ethsrc[i]);
	}
	kprintf("%02x) -> (", pkt->net_ethsrc[5]);

	for(i = 0; i < 5; i++) {
		kprintf("%02x:", pkt->net_ethdst[i]);
	}
	kprintf("%02x), ", pkt->net_ethdst[5]);

	if(ntohs(pkt->net_ethtype) != ETH_IPV6) {
		kprintf("ethtype %x\n", ntohs(pkt->net_ethtype));
		return;
	}

	kprintf("IPv6 (");
	ip_print(pkt->net_ipsrc); kprintf(") -> (");
	ip_print(pkt->net_ipdst); kprintf("), ");
	kprintf("iplen %d, ", ntohs(pkt->net_iplen));
	kprintf("hoplim %d, ", pkt->net_iphl);

	switch(pkt->net_ipnh) {

		case IP_ICMPV6:
		 kprintf("ICMPv6, ");
		 switch(pkt->net_ictype) {

		  case ICMP_ECHOREQST:
			  kprintf("Echo request id 0x%x seq 0x%x ",
					  ntohs(*((uint16*)pkt->net_icdata)), ntohs(*((uint16*)(pkt->net_icdata+2))));
			  break;

		  case ICMP_ECHOREPLY:
			  kprintf("Echo reply id 0x%x seq 0x%x ",
					  ntohs(*((uint16*)pkt->net_icdata)), ntohs(*((uint16*)(pkt->net_icdata+2))));
			  break;

		  case ICMP_TYPE_NBRSOL:
			  nbrsol = (struct nd_nbrsol *)pkt->net_icdata;
			  kprintf("Nbr sol. target ");
			  ip_print(nbrsol->tgtaddr);
			  break;

		  case ICMP_TYPE_NBRADV:
			  nbradv = (struct nd_nbradv *)pkt->net_icdata;
			  kprintf("Nbr adv. target ");
			  ip_print(nbradv->tgtaddr);
			  break;

		  default:
			  kprintf("type %x code %x ", pkt->net_ictype, pkt->net_iccode);
		 }
		 break;

		case IP_UDP:
		 kprintf("UDP (%d) -> (%d) ", ntohs(pkt->net_udpsport), ntohs(pkt->net_udpdport));
		 kprintf("len %d ", ntohs(pkt->net_udplen));
		 break;

		case IP_TCP:
		 kprintf("TCP (%d) -> (%d) ", ntohs(pkt->net_tcpsport), ntohs(pkt->net_tcpdport));
		 uint16 code = ntohs(pkt->net_tcpcode);
		 if(code & TCPF_SYN) kprintf("syn ");
		 if(code & TCPF_FIN) kprintf("fin ");
		 kprintf("seq 0x%x ", ntohl(pkt->net_tcpseq));
		 if(code & TCPF_ACK) kprintf("ack %x ", ntohl(pkt->net_tcpack));
		 break;

		default:
		 kprintf("next header %x ", pkt->net_ipnh);
		 break;
	}
	kprintf("\n");

}
