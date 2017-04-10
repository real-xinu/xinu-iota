/* testbed.c */

#include <xinu.h>

struct	tbinfo testbed;

struct	tmapentry tbedmap[MAX_BBB];

process testbed_proc(void);
/*------------------------------------------------------------------------
 * testbed_init  -  Initialize the data structure for testbed
 *------------------------------------------------------------------------
 */
void	testbed_init (void) {

	int32	i;
	intmask	mask;

	mask = disable();

	testbed.state = 0;

	testbed.nextid = 0;

	testbed.thead = testbed.ttail = testbed.tcount = 0;

	testbed.tsem = semcreate(0);

	testbed.seq = 0;

	for(i = 0; i < MAX_BBB; i++) {
		tbedmap[i].state = TMAP_STATE_FREE;
	}

	resume(create(testbed_proc, 8192, NETPRIO, "testbed process", 0));

	restore(mask);
}

/*------------------------------------------------------------------------
 * testbed_proc  -  Testbed process
 *------------------------------------------------------------------------
 */
process	testbed_proc (void) {

	struct	tbedpacket *pkt;	/* Testbed packet	*/
	struct	tbedpacket *rcvpkt;	/* Received packet	*/
	struct	tbreq req;		/* Testbed request	*/
	int32	retries;		/* No. of retries	*/
	umsg32	msg;			/* Message received	*/
	uint32	time1;			/* Time in ms		*/
	intmask	mask;			/* Interrupt mask	*/
	int32	i;			/* Loop index		*/
	/* Allocate a packet */

	pkt = (struct tbedpacket *)getbuf(netbufpool);

	while(1) {

		/* Wait until there is a request in the queue */

		wait(testbed.tsem);
		kprintf("testbed_proc: request..\n");

		/* Extact the request at the front */

		mask = disable();
		req = testbed.reqq[testbed.thead++];
		if(testbed.thead >= TB_QSIZE) {
			testbed.thead = 0;
		}
		restore(mask);

		/* Increment the sequence number */

		++testbed.seq;

		/* Take action according to type of request */

		switch(req.type) {

		/* Assign message */
		case TBR_TYPE_ASSIGN:
			kprintf("testbed_proc: ASSIGN\n");
			pkt->amsg.amsgtyp = htonl(A_ASSIGN);
			pkt->amsg.anodeid = htonl(req.nodeid);
			kprintf("\tnodeid: %d\n", ntohl(pkt->amsg.anodeid));
			memcpy(pkt->amsg.amcastaddr, req.mcast, 6);
			for(i = 0; i < 46; i++) {
				pkt->amsg.link_info[i].lqi_low = req.link_info[i].lqi_low;
				pkt->amsg.link_info[i].lqi_high = req.link_info[i].lqi_high;
				pkt->amsg.link_info[i].probloss = req.link_info[i].probloss;
			}
			break;

		case TBR_TYPE_NPING:
			kprintf("testbed_proc: NPING\n");
			pkt->amsg.amsgtyp = htonl(A_PING);
			pkt->amsg.anodeid = htonl(req.nodeid);
			break;

		case TBR_TYPE_NPING_ALL:
		case TBR_TYPE_NPINGALL:
			kprintf("testbed_proc: NPING_ALL\n");

			/* There has to be someone waiting */

			if(!req.waiting) {
				continue;
			}

			/* Fill out the packet fields */

			if(req.type == TBR_TYPE_NPING_ALL) {
				pkt->amsg.amsgtyp = htonl(A_PING_ALL);
			}
			else {
				pkt->amsg.amsgtyp = htonl(A_PINGALL);
			}

			pkt->amsg.anodeid = 0;
			memcpy(pkt->ethdst, NetData.ethbcast, 6);
			memcpy(pkt->ethsrc, NetData.ethucast, 6);
			pkt->ethtype = htons(ETH_TYPE_A);
			pkt->amsg.aseq = testbed.seq;

			/* Set the testbed process in waiting state */

			mask = disable();
			testbed.state = TB_STATE_WAIT;
			memset(testbed.waitaddr, 0, 6);
			testbed.pid = getpid();
			write(ETHER0, (char *)pkt, sizeof(struct tbedpacket));

			/* Receive all packets for next 500 ms */

			time1 = clktimems + 500;
			while(clktimems < time1) {
				msg = recvtime(time1-clktimems);
				kprintf("testbed_proc: received %x\n", msg);
				if((int32)msg == SYSERR || (int32)msg == TIMEOUT) {
					send(req.waitpid, SYSERR);
					break;
				}
				rcvpkt = (struct tbedpacket *)msg;

				if(rcvpkt->amsg.aacktyp == ntohl(A_PING_ALL)) {
					kprintf("testbed_proc: PING_ALL\n");
					send(req.waitpid, ntohl(rcvpkt->amsg.anodeid));
				}
				else if(rcvpkt->amsg.aacktyp == ntohl(A_PINGALL)) {
					kprintf("testbed_proc: PINGALL\n");
					for(i = 0; i < MAX_BBB; i++) {
						if(!memcmp(rcvpkt->ethsrc, bbb_macs[i], 6)) {
							send(req.waitpid, i);
							break;
						}
					}
				}
				else {
					kprintf("testbed_proc: unknown ack type %d\n", ntohl(rcvpkt->amsg.aacktyp));
				}
				freebuf((char *)rcvpkt);
			}
			continue;

		default:
			continue;
		}

		/* Fill out fields in the packet */

		pkt->amsg.aseq = testbed.seq;
		memcpy(pkt->ethdst, req.dst, 6);
		memcpy(pkt->ethsrc, NetData.ethucast, 6);
		pkt->ethtype = htons(ETH_TYPE_A);

		mask = disable();
		testbed.state = TB_STATE_WAIT;
		memcpy(testbed.waitaddr, req.dst, 6);
		testbed.pid = getpid();
		restore(mask);

		retries = 0;
		while(retries < 3) {

			retries++;

			/* Send the packet */

			kprintf("testbed_proc: sending assign message\n");
			write(ETHER0, (char *)pkt, sizeof(struct tbedpacket));

			/* Wait for an appropriate response */

			msg = recvtime(1000);
			if((int32)msg == TIMEOUT) {
				continue;
			}
			else {
				freebuf((char *)msg);
				break;
			}
		}

		if(req.waiting) {
			if(retries < 3) {
				send(req.waitpid, OK);
			}
			else {
				send(req.waitpid, SYSERR);
			}
		}

		mask = disable();
		testbed.state = 0;
		restore(mask);
	}

	return OK;
}

/*------------------------------------------------------------------------
 * testbed_in  -  Handle an incoming testbed packet
 *------------------------------------------------------------------------
 */
void	testbed_in (
		struct	tbedpacket *pkt	/* Incoming packet	*/
		)
{
	struct	tbreq rqptr;	/* Testbed request	*/
	uint32	nodeid;		/* Node ID		*/
	int32	i, j;		/* Loop indexes		*/

	byte	allzeros[] = {0, 0, 0, 0, 0, 0};

	/* Change type to host byte order */

	pkt->amsg.amsgtyp = ntohl(pkt->amsg.amsgtyp);

	switch(pkt->amsg.amsgtyp) {

	case A_JOIN:

		if(online) {
			kprintf("====> Incoming Join message\n");
			testbed_assign(-1, pkt->ethsrc);
		}

		freebuf((char *)pkt);
		break;

	case A_ACK:

		kprintf("====> Incoming Ack message\n");

		/* Check if the ack matches our expectation */

		if( (testbed.state == TB_STATE_WAIT) &&
		    (!memcmp(testbed.waitaddr, allzeros, 6) ||
		     !memcmp(pkt->ethsrc, testbed.waitaddr, 6)) &&
		    (pkt->amsg.aseq == testbed.seq) ) {
			kprintf("\t");
			for(i = 0; i < 6; i++) {
				kprintf("%02x ", pkt->ethsrc[i]);
			}
			kprintf("\n");
			send(testbed.pid, (umsg32)pkt);
		}
		else {
			kprintf("\tdoes not match\n");
			kprintf("\tseq = %d, %d\n\t", pkt->amsg.aseq, testbed.seq);
			for(i = 0; i < 6; i++) {
				kprintf("%02x ", pkt->ethsrc[i]);
			}
			kprintf("\n");
		}
		break;

	default:
		freebuf((char *)pkt);
		break;
	}
}
