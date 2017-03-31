#include <xinu.h>

/*-------------------------------------------------------------
 * This function is used to send a broadcast settime message 
 * to all the nodes in current active topology */

status settime(uint32 ctime){

    struct etherPkt *ctime_msg;
    ctime_msg = create_etherPkt();
    int32 retval;
    /* fill out the Ethernet packet fields */
    memcpy ( ctime_msg->src, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( ctime_msg->dst, NetData.ethbcast, ETH_ADDR_LEN );
    ctime_msg->type = htons ( ETH_TYPE_A );
    ctime_msg->msg.amsgtyp = htonl ( A_SETTIME ); /* SETTIME message */
   
    ctime_msg->msg.ctime = ctime;
     
    /*send packet over Ethernet */
    retval = write ( ETHER0, ( char * ) ctime_msg, sizeof ( struct etherPkt ) );
    freemem ( ( char * ) ctime_msg, sizeof ( struct etherPkt ) );

    if ( retval > 0 )
        return OK;

    else
        return SYSERR;


}
