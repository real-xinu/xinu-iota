/* monitor.c - monitor_init, monitor_in */

#include <xinu.h>

struct	_filter mfilter;

/*------------------------------------------------------------------------
 * monitor_init  -  Initialize the monitoring system
 *------------------------------------------------------------------------
 */
void	monitor_init (void) {

	mfilter.state = MFILTER_INACTIVE;
	mfilter.udpslot = -1;
}

/*------------------------------------------------------------------------
 * monitor_in  -  Handle incoming Type B Frames for monitoring
 *------------------------------------------------------------------------
 */
void	monitor_in (
		struct	netpacket_r *pkt,
		int32	pktlen
		)
{
	struct	monitor_msg *mmsg;

	if(mfilter.state == MFILTER_INACTIVE) {
		freebuf((char *)pkt);
		return;
	}

	if(mfilter.nodeid != ntohl(pkt->net_nodeid)) {
		freebuf((char *)pkt);
		return;
	}

	// More filtering

	mmsg = (struct monitor_msg *)getmem(1500);

	mmsg->pktlen = htonl(pktlen-15);
	memcpy(mmsg->pktdata, pkt->net_raddata, pktlen-15);

	udp_send(mfilter.udpslot, (char *)mmsg, pktlen-15+4);

	freemem((char *)mmsg, 1500);
	freebuf((char *)pkt);
}
