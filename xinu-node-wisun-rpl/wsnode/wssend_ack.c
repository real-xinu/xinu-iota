#include <xinu.h>
#include <stdlib.h>

/*-------------------------------------------------------------
 * Send ack message as a repsonse of assign and ping messages
 * ----------------------------------------------------------*/
status wsnode_sendack ( struct netpacket_e *node_msg )
{
    struct netpacket_e *ack_msg;
    ack_msg = (struct netpacket_e *)create_etherPkt (node_msg);
    int32 retval;
    ack_msg->msg.amsgtyp = htonl ( A_ACK );
    //ack_msg->msg.anodeid = htonl ( info.nodeid );
    memcpy ( ack_msg->msg.aacking, ( char * ) ( node_msg ) + 14, 16 );
    ack_msg->msg.anodeid = htonl (info.nodeid);

    /*DEBUG */
    //int i;
    /*DEBUG *//*   for (i=0; i<16; i++)
       {
           kprintf("aacking: %d\n", ack_msg->aacking[i]);
       }*/
    /*DEBUG */
    retval = write ( ETHER0, ( char * ) ack_msg, sizeof ( struct netpacket_e ) );
    freebuf ((char *)node_msg);

    if ( retval > 0 )
        return OK;

    else
        return SYSERR;
}


