#include <xinu.h>

/*-----------------------------------------------------------------
 * message handler is used to call appropiate function based
 * on a message which is received from a node.
 * ----------------------------------------------------------------*/
#if 0
void  amsg_handler ( struct netpacket *pkt )
{
    struct etherPkt *node_msg;
    status retval;
    node_msg = ( struct etherPkt * ) pkt;
    /* -------------------------------------------
     * Extract message type for TYPE A frames
     * ------------------------------------------*/
    int32 amsgtyp = ntohl ( node_msg->msg.amsgtyp );

    /*DEBUG*/ //kprintf("type:%d\n", amsgtyp);

    switch ( amsgtyp ) {
        case A_JOIN:
            kprintf ( "====>Join message is received\n" );

            if ( online ) {
                retval = wsserver_assign ( pkt );

                if ( retval == OK ) {
                    kprintf ( "<====Assign message is sent\n" );
                }

            } else
                freebuf ( ( char * ) pkt );

            break;

        case A_ACK:
            ack_handler ( pkt );
            break;

        case A_ERR:
            wsserver_senderr ( pkt );
            kprintf ( "====>Error message is received\n" );
            break;

        default:
            freebuf ( ( char * ) pkt );
            break;
    }

    return;
}
#endif

