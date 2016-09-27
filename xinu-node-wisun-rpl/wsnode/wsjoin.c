#include <xinu.h>

/*------------------------------------------------
 * Send a join broadcast message when the node boots up.
 * -------------------------------------------------*/
status wsnode_join()
{
    struct netpacket_e *join_msg;
    int32 retval;
    join_msg = ( struct netpacket_e * ) getmem ( sizeof ( struct netpacket_e) );
    /*fill out Ethernet packet header fields */
    memset ( join_msg, 0, sizeof ( join_msg ) );
    memcpy ( join_msg->net_ethsrc, iftab[0].if_hwucast, ETH_ADDR_LEN );
    memcpy ( join_msg->net_ethdst, iftab[0].if_hwbcast, ETH_ADDR_LEN );
    join_msg->net_ethtype = htons ( ETH_TYPE_A );
    /*fill out Ethernet packet data fields */
    join_msg->msg.amsgtyp = htonl ( A_JOIN );
    join_msg->msg.anodeid = htons ( 0 );
    retval = write ( ETHER0, ( char * ) join_msg, sizeof (struct netpacket_e));
    freemem ((char *) join_msg , sizeof (struct netpacket_e));

    if ( retval > 0 ) {
        return OK;

    } else
        return SYSERR;
}
