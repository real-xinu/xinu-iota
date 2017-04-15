/* amsg.c - amsg_in */

#include <xinu.h>

/*------------------------------------------------------------------------
 * amsg_in  -  Handle incoming A Type messages
 *------------------------------------------------------------------------
 */
void	amsg_in (
		struct	netpacket_a *pkt
		)
{
	uint32	ipaddr;

	switch(ntohl(pkt->amsg.amsgtyp)) {

		case A_CLEANUP:
			panic("Node is going down");
			break;

		case A_MONITOR:
			if(ntohl(pkt->amsg.mon_cmd) == MCMD_STOP) {
				kprintf("Monitoring stopped\n");
				mfilter.state = MFILTER_INACTIVE;
			}
			else if(ntohl(pkt->amsg.mon_cmd) == MCMD_START) {
				kprintf("Monitoring started\n");
				mfilter.nodeid = ntohl(pkt->amsg.mon_nodeid);
				mfilter.fsrc = pkt->amsg.mon_fsrc;
				mfilter.fdst = pkt->amsg.mon_fdst;
				memcpy(mfilter.ipsrc, pkt->amsg.mon_ipsrc, 16);
				memcpy(mfilter.ipdst, pkt->amsg.mon_ipdst, 16);
				if(mfilter.udpslot == -1) {
					dot2ip(WIRESHARK_IP, &ipaddr);
					mfilter.udpslot = udp_register(ipaddr, WIRESHARK_PORT, 33000);
				}
				mfilter.state = MFILTER_ACTIVE;
			}
			break;
	}

	freebuf((char *)pkt);
	return;
}
