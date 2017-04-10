/* wsserver.c - Wi-SUN emulation testbed server */

#include <xinu.h>

int32	nodeid = 0;
int32	nnodes = 0;
byte	ack_info[16];
int32	ping_ack_flag[MAXNODES];
int32	online = 1;
int32	*seqnum;

/*-----------------------------------------------------------------
 * Network topology data strucutres which are used to keep current
 * and old network topology information in the RAM.
 * --------------------------------------------------------------*/
struct topo_entry topo[MAXNODES];
struct topo_entry old_topo[MAXNODES];

/*--------------------------------------------------------------------
 * Create an Ethernet packet and allocate memory
*--------------------------------------------------------------------*/
struct etherPkt *create_etherPkt()
{
    struct etherPkt *msg;
    msg = ( struct etherPkt * ) getmem (sizeof (struct etherPkt));
    return msg;
}

/*------------------------------------------------------------------------
 * wsserver  -  Server to manage the Wi-SUN emulation testbed
 *------------------------------------------------------------------------
 */
process	wsserver (void)
{
	struct	c_msg ctlpkt;	/* Control Packet	*/
	struct	c_msg *replypkt;/* Reply Packet		*/
	uint32	remip;		/* Remote IP address	*/
	uint16	remport;	/* Remote port		*/
	int32	retval;		/* Return Value		*/
	int32	slot;		/* UDP slot		*/
	int32	i;		/* Loop index		*/

	kprintf ("=========== Started Wi-SUN testbed server ===========\n");

	/* Register UDP port for use by server */

	slot = udp_register (0, 0, WS_PORT);

	if (slot == SYSERR) {
		kprintf("testbed server could not get UDP port %d\n", WS_PORT);
		return SYSERR;
	}

	while ( TRUE ) {

		/* Attempt to receive control packet */
		
		retval = udp_recvaddr(slot, &remip, &remport, (char * )&ctlpkt,
                                sizeof ( ctlpkt ), WS_TIMEOUT );

		if ( retval == TIMEOUT ) {
			continue;
		}
		else if ( retval == SYSERR ) {
			kprintf ( "WARNING: UDP receive error in testbed server\n" );
		}
		else {
			kprintf ( "*====> Got control message %d\n", ntohl(ctlpkt.cmsgtyp) );
			replypkt = cmsg_handler (&ctlpkt );
			
			if (replypkt->cmsgtyp != htonl (C_SHUTDOWN)) {
				retval = udp_sendto ( slot, remip, remport,
                                             ( char * ) replypkt, sizeof ( struct c_msg ) );
				
				if ( retval == SYSERR ) {
					kprintf ( "WARNING: UDP send error in testbed server\n" );
				}
				else {
					kprintf ( "* <==== Replied with %d\n", ntohl(replypkt->cmsgtyp));
				}
				
				freemem ( ( char * ) replypkt, sizeof ( struct c_msg ) );
			}
		}
		for (i =0; i< MAXNODES; i++) {
			ping_ack_flag[i] = 0;
		}
		for (i =0; i< MAX_BBB; i++) {
			bbb_stat[i] = 0;
		}
	}
}



