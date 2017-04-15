/* monitor.c - monitor */

#include <xinu.h>

/*------------------------------------------------------------------------
 * monitor  -  Handle a monitoring request from management app
 *------------------------------------------------------------------------
 */
void	monitor (
		struct	c_msg *cpkt
		)
{
	struct	etherPkt *pkt;

	pkt = (struct etherPkt *)getmem(sizeof(struct etherPkt));

	memcpy(pkt->dst, testbed.mon_ethaddr, 6);
	memcpy(pkt->src, NetData.ethucast, 6);
	pkt->type = htons(ETH_TYPE_A);
	pkt->msg.amsgtyp = htonl(A_MONITOR);
	pkt->msg.mon_nodeid = cpkt->mon_nodeid;
	pkt->msg.mon_cmd = cpkt->mon_cmd;
	pkt->msg.mon_fsrc = cpkt->mon_fsrc;
	pkt->msg.mon_fdst = cpkt->mon_fdst;
	memcpy(pkt->msg.mon_ipsrc, cpkt->mon_ipsrc, 16);
	memcpy(pkt->msg.mon_ipdst, cpkt->mon_ipdst, 16);

	write(ETHER0, (char *)pkt, sizeof(struct etherPkt));
}
