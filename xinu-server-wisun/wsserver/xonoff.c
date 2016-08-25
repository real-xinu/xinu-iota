#include <xinu.h>
/*-----------------------------------------------------------------
 * X_OFF and X_ON Function
 * ---------------------------------------------------------------*/
status wsserver_xonoff (int cmd, int xonoffid)
{
    struct etherPkt *xonoff_msg;
    xonoff_msg = create_etherPkt();
    int32 retval;
    xonoff_msg->type = htons ( ETH_TYPE_A );

    if (cmd == 0) {
        xonoff_msg->msg.amsgtyp = htonl (A_XOFF);

    } else
        xonoff_msg->msg.amsgtyp = htonl (A_XON);

    if (xonoffid == -1) {
        xonoff_msg->msg.anodeid = 0;
        memcpy ( xonoff_msg->src, NetData.ethucast, ETH_ADDR_LEN );
        memcpy ( xonoff_msg->dst, NetData.ethbcast, ETH_ADDR_LEN );

    } else {
        xonoff_msg->msg.anodeid = xonoffid;
        memcpy ( xonoff_msg->src, NetData.ethucast, ETH_ADDR_LEN );
        memcpy ( xonoff_msg->dst, topo[xonoffid].t_macaddr, ETH_ADDR_LEN );
    }

    retval = write ( ETHER0, ( char * ) xonoff_msg, sizeof ( struct etherPkt ) );
    freemem ( ( char * ) xonoff_msg, sizeof ( struct etherPkt ) );

    if ( retval > 0 ) {
        return OK;

    } else
        return SYSERR;
}


