/* wsserver.c - Wi-SUN emulation testbed server */

#include <xinu.h>
#include <stdio.h>
/*------------------------------------------------------------------
 a data strucure to keep assigned multicast address and the node ID.
*------------------------------------------------------------------*/
struct node_info {
    int32 nodeid;
    byte mcastaddr[6];
};

struct node_info info;
/*-----------------------------------------------------------
* Create and Ethernet packet and fill out header fields
*----------------------------------------------------------*/
struct etherPkt *create_etherPkt ( struct etherPkt *pkt )
{
    struct etherPkt *msg;
    msg = ( struct etherPkt * ) getmem ( sizeof ( struct etherPkt ) );
    /*fill out Ethernet packet header fields */
    memset ( msg, 0, sizeof ( msg ) );
    
    memcpy ( msg->src, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( msg->dst, pkt->src, ETH_ADDR_LEN );
    msg->type = htons ( ETH_TYPE_A );
    return msg;
}
/*------------------------------------------------
 * Send a join broadcast message when the node boots up.
 * -------------------------------------------------*/
status wsnode_join()
{
    struct etherPkt *join_msg;
    int32 retval;
    join_msg = ( struct etherPkt * ) getmem ( sizeof ( struct etherPkt ) );
    /*fill out Ethernet packet header fields */
    memset ( join_msg, 0, sizeof ( join_msg ) );
    memcpy ( join_msg->src, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( join_msg->dst, NetData.ethbcast, ETH_ADDR_LEN );
    join_msg->type = htons ( ETH_TYPE_A );
    /*fill out Ethernet packet data fields */
    join_msg->amsgtyp = htonl ( A_JOIN );
    join_msg->anodeid = htons ( 0 );
    retval = write ( ETHER0, ( char * ) join_msg, sizeof ( struct etherPkt ) );
    freemem((char *) join_msg , sizeof(struct etherPkt));
    if ( retval > 0 )
        return OK;

    else
        return SYSERR;
}
/*-------------------------------------------------------------
 * Send ack message as a repsonse of assign and ping messages
 * ----------------------------------------------------------*/
status wsnode_sendack ( struct etherPkt *node_msg )
{
    struct etherPkt *ack_msg;
    //struct etherPkt *node_msg;
    //node_msg = ( struct etherPkt * ) pkt;
    ack_msg = create_etherPkt (node_msg);
    int32 retval;
    ack_msg->amsgtyp = htonl ( A_ACK );
    ack_msg->anodeid = htonl ( info.nodeid );
    memcpy ( ack_msg->aacking, ( char * ) ( node_msg ) + 14, 16 );
    //int i;
 /*   for (i=0; i<16; i++)
    {
        kprintf("aacking: %d\n", ack_msg->aacking[i]);
    }*/
    retval = write ( ETHER0, ( char * ) ack_msg, sizeof ( struct etherPkt ) );
    freebuf((char *)node_msg);
    if ( retval > 0 )
        return OK;

    else
        return SYSERR;
}
/*-------------------------------------------------------------------
 A utility function to print multicast address and node ID
-----------------------------------------------------------------*/
void print_info()
{
    int i, j;
    kprintf ( "node id:%d\n", info.nodeid );

    for ( j = 0; j < 6; j++ ) {
        for ( i = 7; i >= 0; i-- ) {
    //         kprintf("%d ", (info.mcastaddr[j]>> i) &0x01);
        }

      //  kprintf(" ");
    }
}


/*------------------------------------------------
 * Message handler is used to call
 * appropiate function based on message type.
 * -----------------------------------------------*/
void amsg_handler ( struct etherPkt *node_msg )
{
    float delay;
    //struct etherPkt *node_msg;
    //node_msg = ( struct etherPkt * ) pkt;
    int32 amsgtyp = ntohl ( node_msg->amsgtyp );
    int i;
    /*for (i=0; i< 6; i++) 
    {
       kprintf("%02x:", node_msg->src[i]);


    }
    kprintf("\n");

   for (i=0; i< 6; i++) 
    {
       kprintf("%02x:", node_msg->dst[i]);


    }
    kprintf("\n");*/

    //kprintf("A msg type:%d\n", amsgtyp);
    /*--------------------------------------------------------------------------
     * The message type for type A frames  should be check here an appropiate 
     * function should be called to handle the request.
     * -----------------------------------------------------------------------*/
    switch ( amsgtyp ) {
        case A_ASSIGN:
            info.nodeid = ntohl ( node_msg->anodeid );

            if ( wsnode_sendack (node_msg) == OK ) {
                for ( i = 0; i < 6; i++ ) {
                    info.mcastaddr[i] = node_msg->amcastaddr[i];
                }

                print_info();
                kprintf ( "\n====>ACK message is sent\n" );
            }

            break;

        case A_RESTART:
            kprintf ( "<==== RESTART message is received\n" );
            break;

        case A_XOFF:
            kprintf ( "<====XOF message is received\n" );
            break;

        case A_XON:
            kprintf ( "<==== XON message is received\n" );
            break;

        case A_PING:
            kprintf ( "<==== PING message is received\n" );

            if ( wsnode_sendack (node_msg) == OK ) {
                kprintf ( "====> ACK message is sent\n" );
            }

            break;

        case A_PING_ALL:
            delay = ( info.nodeid * 0.001 );
            sleep ( delay );
            kprintf ( "<==== PINGALL message is received\n" );

            if ( wsnode_sendack (node_msg ) == OK ) {
                kprintf ( "====> ACK message is sent\n" );
            }

            break;
    }
}


