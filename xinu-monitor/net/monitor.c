/* monitor.c - monitor_init, monitor_in */

#include <xinu.h>

struct	_filter mfilter;

/*------------------------------------------------------------------------
 * monitor_init  -  Initialize the monitoring data structures
 *------------------------------------------------------------------------
 */
void	monitor_init (void) {

	mfilter.state = MFILTER_INACTIVE;
	mfilter.udpslot = -1;
}

/*------------------------------------------------------------------------
 * monitor_in  -  Handle incoming TYPE B Frames for monitoring
 *------------------------------------------------------------------------
 */
void	monitor_in (
		struct	netpacket_r *pkt,
		uint32	pktlen
		)
{
	struct	monitor_msg *mmsg;
	int32	mmsglen;

	if(mfilter.state == MFILTER_INACTIVE) {
		freebuf((char *)pkt);
		return;
	}

	if(ntohl(pkt->net_nodeid) != mfilter.nodeid) {
		freebuf((char *)pkt);
		return;
	}

	kprintf("monitor:: pkt. nodeid: %d\n", ntohl(pkt->net_nodeid));
	mmsglen = 4 + pktlen - 15;

	mmsg = (struct monitor_msg *)getmem(1500+4);

	mmsg->pktlen = htonl(pktlen-15);
	memcpy(mmsg->pktdata, pkt->net_raddata, pktlen-15);

	udp_send(mfilter.udpslot, (char *)mmsg, mmsglen);

	freemem((char *)mmsg, 1504);
	freebuf((char *)pkt);

	return;
}
