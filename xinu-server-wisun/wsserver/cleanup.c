#include <xinu.h>

/* Send a broadcast message to all nodes to shut them down */


status cleanup()
{
    struct etherPkt *clean_msg;
    clean_msg = create_etherPkt();
    int32 retval;
    /* fill out the Ethernet packet fields */
    memcpy ( clean_msg->src, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( clean_msg->dst, NetData.ethbcast, ETH_ADDR_LEN );
    clean_msg->type = htons ( ETH_TYPE_A );
    clean_msg->msg.amsgtyp = htonl ( A_CLEANUP );
   
    /*send packet over Ethernet */
    retval = write ( ETHER0, ( char * ) clean_msg, sizeof ( struct etherPkt ) );
    freemem ( ( char * ) clean_msg, sizeof ( struct etherPkt ) );

    if ( retval > 0 )
        return OK;

    else
        return SYSERR;

}
