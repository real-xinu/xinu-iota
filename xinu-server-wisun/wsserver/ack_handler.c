#include <xinu.h>

/*----------------------------------------------------------------
 *  ACK handler
 *  -------------------------------------------------------------*/
void ack_handler ( struct netpacket *pkt )
{
    struct etherPkt *node_msg;
    node_msg = ( struct etherPkt * ) pkt;
    int i, ack;
    ack = 0;

    for ( i = 0; i < 16; i++ ) {
        if ( node_msg->msg.aacking[i] == ack_info[i] ) {
            ack++;
        }

        /*DEBUG*/ //kprintf("aacking:%d:%d\n", node_msg->aacking[i],ack_info[i]);
    }

    i = ntohl ( node_msg->msg.anodeid );

    if ( ack == 16 ) {
        if ( ack_info[3] == A_ASSIGN ) {
            topo_update_mac ( pkt );
            kprintf ( "====>Assign ACK message is received\n" );

        } else if ( ack_info[3] == A_PING ) {
            ping_ack_flag[i] = 1;
            freebuf ( ( char * ) pkt );
            kprintf ("--->Ping ACK message is received\n");

        } else if ( ack_info[3] == A_PING_ALL ) {
            ping_ack_flag[i] = 1;
            freebuf ( ( char * ) pkt );
        }

    } else
        freebuf ( ( char * ) pkt );
}


