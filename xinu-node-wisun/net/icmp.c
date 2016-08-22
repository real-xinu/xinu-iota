/* icmp.c - icmp_in */

#include <xinu.h>

/*------------------------------------------------------------------------
 * icmp_in  -  Handle incoming ICMP packets
 *------------------------------------------------------------------------
 */
void	icmp_in (
		struct	netpacket *pkt	/* Packet buffer pointer	*/
		)
{
	//kprintf("Incoming ICMP packet: type %02x, code %02x\n", pkt->net_ictype, pkt->net_iccode);

	//kprintf("ICMP cksum: %04X, %04X\n", pkt->net_iccksum, icmp_cksum(pkt));

	/* Verify ICMP checksum */
	if(icmp_cksum(pkt) != 0) {
		return;
	}

	switch(pkt->net_ictype) {

	case ICMP_TYPE_ERQ:

		icmp_send(ICMP_TYPE_ERP, 0, pkt->net_ipsrc, pkt->net_icdata,
				pkt->net_iplen - 4, pkt->net_iface);
		break;

	case ICMP_TYPE_NS:
	case ICMP_TYPE_NA:
		//kprintf("Incoming nd packet\n");
		nd_in(pkt);
		break;

	default:
		break;

	}
}

/*------------------------------------------------------------------------
 * icmp_send  -  Send an ICMP packet
 *------------------------------------------------------------------------
 */
int32	icmp_send (
		byte	ictype,		/* ICMP type		*/
		byte	iccode,		/* ICMP code		*/
		byte	ipdst[],	/* IP destination	*/
		void	*icdata,	/* ICMP data		*/
		int32	datalen,	/* ICMP data length	*/
		int32	iface		/* Interface index	*/
		)
{
	struct	netpacket *pkt;	/* Packet buffer pointer	*/

	/* Allocate buffer for the packet */
	pkt = (struct netpacket *)getbuf(netbufpool);
	if((int32)pkt == SYSERR) {
		return SYSERR;
	}

	/* If Ip dest is link-local, an interface must be provided */
	if(isipllu(ipdst) && iface == -1) {
		return SYSERR;
	}

	memset(pkt, 0, PACKLEN);

	/* Set the IP, ICMP fields in the packet */
	pkt->net_iface = iface;

	pkt->net_ipvtch = 0x60;
	pkt->net_ipnh = IP_ICMP;
	pkt->net_iphl = 255;
	pkt->net_iplen = 4 + datalen;
	memcpy(pkt->net_ipdst, ipdst, 16);

	pkt->net_ictype = ictype;
	pkt->net_iccode = iccode;
	memcpy(pkt->net_icdata, icdata, datalen);

	//kprintf("icmp_send: calling ip_send\n");
	ip_send(pkt);

	return OK;
}

/*------------------------------------------------------------------------
 * icmp_cksum  -  Compute checksum of an ICMP packet
 *------------------------------------------------------------------------
 */
uint16	icmp_cksum (
		struct	netpacket *pkt	/* Packet buffer pointer	*/
		)
{
	uint32	cksum;	/* 32-bit sum of all shorts	*/
	uint16	*ptr16;	/* Pointer to a short		*/
	int32	i;	/* Index variable		*/

	/* This is the pseudo header */
	#pragma pack(1)
	struct	{
		byte	ipsrc[16];
		byte	ipdst[16];
		uint32	icmplen;
		byte	zero[3];
		byte	ipnh;
	} pseudo;
	#pragma pack()

	/* Initialize the pseudo header */
	memset(&pseudo, 0, sizeof(pseudo));
	memcpy(pseudo.ipsrc, pkt->net_ipsrc, 16);
	memcpy(pseudo.ipdst, pkt->net_ipdst, 16);
	pseudo.icmplen = htonl(pkt->net_iplen);
	pseudo.ipnh = IP_ICMP;

	cksum = 0;
	ptr16 = (uint16 *)&pseudo;

	/* First add all shorts in the pseudo header */
	for(i = 0; i < sizeof(pseudo); i = i + 2) {
		cksum += htons(*ptr16);
		ptr16++;
	}

	ptr16 = (uint16 *)pkt->net_ipdata;

	/* Add all the shorts in the ICMP packet */
	for(i = 0; i < pkt->net_iplen; i = i + 2) {
		cksum += htons(*ptr16);
		ptr16++;
	}

	cksum = (uint16)cksum + (cksum >> 16);

	return (uint16)~cksum;
}
