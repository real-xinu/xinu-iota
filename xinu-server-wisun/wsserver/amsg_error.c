#include <xinu.h>

/*-------------------------------------------------------------------------
 * Send error message as a response to a node.
 * --------------------------------------------------------------------------*/
status wsserver_senderr ( struct netpacket *pkt )
{
    struct etherPkt *err_msg;
    err_msg = create_etherPkt();
    int32 retval;
    /* fill out the Ethernet packet fields */
    memcpy ( err_msg->src, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( err_msg->dst, pkt->net_ethsrc, ETH_ADDR_LEN );
    err_msg->type = htons ( ETH_TYPE_A );
    err_msg->msg.amsgtyp = htonl ( A_ERR ); /* Error message */
    /*send packet over Ethernet */
    retval = write ( ETHER0, ( char * ) err_msg, sizeof ( struct etherPkt ) );
    freemem ( ( char * ) err_msg, sizeof ( struct etherPkt ) );
    freebuf ( ( char * ) pkt );

    if ( retval > 0 )
        return OK;

    else
        return SYSERR;
}
