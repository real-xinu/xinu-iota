/* monitor.c - monitor_in */

#include <xinu.h>

#define WIRESHARK_IP	"128.10.137.51"
#define	WIRESHARK_PORT	50000

#pragma pack(1)
struct	monitor_msg {
	uint32	pktlen;
	byte	pktdata[];
};
#pragma pack()

/*------------------------------------------------------------------------
 * monitor_in  -  Handle incoming Type B frames for monitoring
 *------------------------------------------------------------------------
 */
void	monitor_in (
		struct	netpacket *pkt,
		int32	totallen
		)
{
	static	struct	monitor_msg *mmsg;
	static	int32	udp_slot = -1;
	uint32	remip;

	if(udp_slot == -1) {

		dot2ip(WIRESHARK_IP, &remip);

		udp_slot = udp_register(remip, WIRESHARK_PORT, 40000);
		if(udp_slot == SYSERR) {
			panic("monitor_in: cannot register UDP slot");
		}

		mmsg = (struct monitor_msg *)getmem(1500-20-8);
		if((int32)mmsg == SYSERR) {
			panic("monitor_in: cannot allocate memory");
		}
	}

	kprintf("monitor_in: sending a UDP message of packet length: %d\n", totallen-14);
	mmsg->pktlen = htonl(totallen-14);
	kprintf("monitor_in: copying to %x, totallen: %d\n", mmsg, totallen);
	memcpy(mmsg->pktdata, &pkt->net_ipvh, totallen-14);

	int32	err;
	err = udp_send(udp_slot, (char *)mmsg, 4+totallen-14);
	kprintf("err = %d\n", err);

	freebuf((char *)pkt);
}
