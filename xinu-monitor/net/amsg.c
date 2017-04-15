/* amsg.c - amsg_in */

#include <xinu.h>

/*------------------------------------------------------------------------
 * amsg_in  -  Handle incoming TYPE A message
 *------------------------------------------------------------------------
 */
void	amsg_in (
		struct	netpacket *pkt
		)
{
	uint32	ipaddr;

	switch(ntohl(pkt->amsg.amsgtyp)) {

		case A_CLEANUP:

			panic("Xinu Monitor is going down");
			break;

		case A_MONITOR:
			kprintf("Received monitoring request: %d, %d\n", pkt->amsg.mon_cmd, ntohl(pkt->amsg.mon_cmd));
			if(ntohl(pkt->amsg.mon_cmd) == MCMD_STOP) {
				kprintf("Stopping monitor\n");
				mfilter.state = MFILTER_INACTIVE;
				break;
			}

			if(ntohl(pkt->amsg.mon_cmd) == MCMD_START) {
				kprintf("Starting monitor\n");
				mfilter.nodeid = ntohl(pkt->amsg.mon_nodeid);
				mfilter.fsrc = pkt->amsg.mon_fsrc;
				mfilter.fdst = pkt->amsg.mon_fdst;
				if(mfilter.fsrc) {
					memcpy(mfilter.ipsrc, pkt->amsg.mon_ipsrc, 16);
				}
				if(mfilter.fdst) {
					memcpy(mfilter.ipdst, pkt->amsg.mon_ipdst, 16);
				}
				if(mfilter.udpslot == -1) {
					dot2ip(WIRESHARK_IP, &ipaddr);
					mfilter.udpslot = udp_register(ipaddr, WIRESHARK_PORT, 30000);
				}
				mfilter.state = MFILTER_ACTIVE;
			}
			break;
	}

	freebuf((char *)pkt);
	return;
}
