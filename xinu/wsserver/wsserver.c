/* wsserver.c - Wi-SUN emulation testbed server */

#include <xinu.h>
#include <stdio.h>

struct c_msg * cmsg_handler(int32 mgm_msgtyp)
{
	struct c_msg *cmsg_reply;
	cmsg_reply = (struct c_msg *) getmem(sizeof(struct c_msg));
	memset(cmsg_reply, 0, sizeof(struct c_msg));

	switch(mgm_msgtyp)
	{
		case C_RESTART:
			printf("Message type is %d\n", C_RESTART);
			cmsg_reply->cmsgtyp = htonl(C_OK);
			break;
		case C_RESTART_NODES:
			printf("Message type is %d\n", C_RESTART_NODES);
			cmsg_reply->cmsgtyp = htonl(C_OK);
			break;
		case C_PING_REQ:
			printf("Message type is %d\n", C_PING_REQ);
			cmsg_reply->cmsgtyp = htonl(C_OK);
			break;
		case C_PING_ALL:
			printf("Message type is %d\n", C_PING_ALL);
			cmsg_reply->cmsgtyp = htonl(C_OK);
			break;
		case C_XOFF:
			printf("Message type is %d\n", C_XOFF);
			cmsg_reply->cmsgtyp = htonl(C_OK);
			break;
		case C_XON:
			printf("Message type is %d\n", C_XON);
			cmsg_reply->cmsgtyp = htonl(C_OK);
			break;
		case C_OFFLINE:
			printf("Message type is %d\n", C_OFFLINE);
			cmsg_reply->cmsgtyp = htonl(C_OK);
			break;
		case C_ONLINE:
			printf("Message type is %d\n", C_ONLINE);
			cmsg_reply->cmsgtyp = htonl(C_OK);
			break;
		case C_TOP_REQ:
			printf("Message type is %d\n", C_TOP_REQ);
			cmsg_reply->cmsgtyp = htonl(C_OK);
			break;
		case C_NEW_TOP:
			printf("Message type is %d\n", C_NEW_TOP);
			cmsg_reply->cmsgtyp = htonl(C_OK);
			break;
		case C_TS_REQ:
			printf("Message type is %d\n", C_TS_REQ);
			cmsg_reply->cmsgtyp = htonl(C_TS_RESP);
			break;
	}

	return cmsg_reply;
}

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

	printf("=== Started Wi-SUN testbed server ===\n");

	/* Register UDP port for use by server */
	slot = udp_register(0, 0, serverport);
	if (slot == SYSERR) {
		printf("testbed server could not get UDP port %d\n", serverport);
		return 1;
	}

	while (TRUE) {
		/* Attempt to receive control packet */
		retval = udp_recvaddr(slot, &remip, &remport, (char *) &ctlpkt,
				sizeof(ctlpkt), timeout);

		if (retval == TIMEOUT) {
			continue;
		} else if (retval == SYSERR) {
			printf("WARNING: UDP receive error in testbed server\n");
			continue; /* may be better to have the server terminate? */
		} else {
			int32 mgm_msgtyp = ntohl(ctlpkt.cmsgtyp);
			printf("* => Got control message %d\n", mgm_msgtyp);

			struct c_msg *replypkt = cmsg_handler(mgm_msgtyp);

			status sndval = udp_sendto(slot, remip, remport,
					(char *) replypkt, sizeof(struct c_msg));
			if (sndval == SYSERR) {
				printf("WARNING: UDP send error in testbed server\n");
			} else {
				int32 reply_msgtyp = ntohl(replypkt->cmsgtyp);
				printf("* <= Replied with %d\n", reply_msgtyp);
			}

			freemem((char *) replypkt, sizeof(struct c_msg));
		}
	}
}
