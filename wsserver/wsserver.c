/* wsserver.c - Wi-SUN emulation testbed server */

#include <xinu.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * wsserver  -  Server to manage the Wi-SUN emulation testbed
 *------------------------------------------------------------------------
 */
process	wsserver ()
{
	uint16 serverport = 55000;
	int32 slot;
	int32 retval;
	uint32 remip;
	uint16 remport;
	uint32 timeout = 600000;
	struct c_msg ctlpkt;

	/* Register UDP port for use by server */
	slot = udp_register(0, 0, serverport);
	if (slot == SYSERR) {
		kprintf("emulation server could not get UDP port %d\n", serverport);
		return 1;
	}

	while (TRUE) {
		/* Attempt to receive control packet */
		retval = udp_recvaddr(slot, &remip, &remport, (char *) &ctlpkt,
				sizeof(ctlpkt), timeout);

		if (retval == TIMEOUT) {
			continue;
		} else if (retval == SYSERR) {
			kprintf("WARNING: UDP receive error in emulation server\n");
			continue; /* may be better to have the server terminate? */
		} else {
			kprintf("Control msg: len %d type %d\n",
					ntohl(ctlpkt.clength), ntohl(ctlpkt.cmsgtyp));
			/* TODO: Actual processing (ctl server does nothing for now) */
		}
	}
}
