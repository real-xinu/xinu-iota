/* wsserver.c - Wi-SUN emulation testbed server */

/*--------------------
 * headers
 * ------------------*/
#include <xinu.h>
#include <stdio.h>
#include <string.h>

int32 nodeid = 0;
int32 nnodes = 0;
byte ack_info[16];
int32 ping_ack_flag[MAXNODES];
int32 online = 1;
int32 *seqnum;

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
process	wsserver ()
{
    int i;
    uint16 serverport = PORT;
    int32 slot;
    int32 retval;
    uint32 remip;
    uint16 remport;
    uint32 timeout = TIME_OUT;
    struct c_msg ctlpkt;
    struct c_msg *replypkt;
    printf ( "=========== Started Wi-SUN testbed server ===========\n" );
    /* Register UDP port for use by server */
    slot = udp_register ( 0, 0, serverport );

    if ( slot == SYSERR ) {
        kprintf ( "testbed server could not get UDP port %d\n", serverport );
        return 1;
    }

    while ( TRUE ) {
        /* Attempt to receive control packet */
        retval = udp_recvaddr ( slot, &remip, &remport, ( char * ) &ctlpkt,
                                sizeof ( ctlpkt ), timeout );

        if ( retval == TIMEOUT ) {
            continue;

        } else if ( retval == SYSERR ) {
            kprintf ( "WARNING: UDP receive error in testbed server\n" );
            

        } else {
            int32 mgm_msgtyp = ntohl ( ctlpkt.cmsgtyp );
            kprintf ( "*====> Got control message %d\n", mgm_msgtyp );
            replypkt = cmsg_handler (&ctlpkt );

            if (replypkt->cmsgtyp != htonl (C_SHUTDOWN)) {
                status sndval = udp_sendto ( slot, remip, remport,
                                             ( char * ) replypkt, sizeof ( struct c_msg ) );

                if ( sndval == SYSERR ) {
                    kprintf ( "WARNING: UDP send error in testbed server\n" );

                } else {
                    int32 reply_msgtyp = ntohl ( replypkt->cmsgtyp );
                    kprintf ( "* <==== Replied with %d\n", reply_msgtyp );
                }

                freemem ( ( char * ) replypkt, sizeof ( struct c_msg ) );
            }
            for (i =0; i< MAXNODES; i++)
	         ping_ack_flag[i] = 0;
            for (i =0; i< MAX_BBB; i++)
	         bbb_stat[i] = 0;
        }
    }
}



