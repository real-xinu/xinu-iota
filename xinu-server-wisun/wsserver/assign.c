#include <xinu.h>

/*-----------------------------------------------------------
 * Send an assign message as a response of JOIN message that is
 * received from a node.
 * ----------------------------------------------------------*/
status wsserver_assign ( struct netpacket *pkt )
{
    if (nodeid >= MAXNODES) {
        freebuf ((char *)pkt);
        return SYSERR;
    }

    struct etherPkt *assign_msg;

    assign_msg = create_etherPkt();

    int32 retval;

    int i;

    /* fill out Ethernet packet fields */
    /*DEBUG */ //for (i=0;i < 6;i++)
    //{
    //kprintf("%02x:", pkt->net_ethsrc[i]);
    //}
    memcpy ( assign_msg->src, NetData.ethucast, ETH_ADDR_LEN );

    memcpy ( assign_msg->dst, pkt->net_ethsrc, ETH_ADDR_LEN );

    assign_msg->type = htons ( ETH_TYPE_A );

    assign_msg->msg.amsgtyp = htonl ( A_ASSIGN ); /*Assign message */

    assign_msg->msg.anodeid = htonl ( nodeid );

    /*DEBUG *///kprintf("Assign type: %d:%d\n", htonl(A_ASSIGN), htonl(nodeid));
    for ( i = 0; i < 6; i++ ) {
        assign_msg->msg.amcastaddr[i] = topo[nodeid].t_neighbors[i];
        /*DEBUG *///kprintf("%02x:", assign_msg->amcastaddr[i]);
    }

    kprintf ( "\n*** Assigned Nodeid***: %d\n", nodeid );
    memset ( ack_info, 0, sizeof ( ack_info ) );
    memcpy ( ack_info, ( char * ) ( assign_msg ) + 14, 16 );
    retval = write ( ETHER0, ( char * ) assign_msg, sizeof (struct etherPkt) );
    freemem ( ( char * ) assign_msg, sizeof (struct etherPkt) );
    freebuf ( ( char * ) pkt );

    if ( retval > 0 ) {
        return OK;

    } else {
        return SYSERR;
    }
}


