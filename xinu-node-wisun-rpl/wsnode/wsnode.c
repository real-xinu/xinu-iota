/* wsserver.c - Wi-SUN emulation testbed server */

#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>



int xon = 1;
int32 slot;
struct node_info info;

/*-------------------------------------------------------------
 *  Send radio packets between the nodes using TYPE_B frames
 *-----------------------------------------------------------*/

process wsnodeapp()
{
	return OK;
}


/*-----------------------------------------------------------
* Create and Ethernet packet and fill out header fields
*----------------------------------------------------------*/
struct netpacket_e *create_etherPkt ( struct netpacket_e *pkt )
{
    struct netpacket_e *msg;
    msg = ( struct netpacket_e * ) getmem ( sizeof ( struct netpacket_e) );
    /*fill out Ethernet packet header fields */
    memset ( msg, 0, sizeof ( msg ) );
    memcpy ( msg->net_ethsrc, iftab[0].if_hwucast, ETH_ADDR_LEN );
    memcpy ( msg->net_ethdst, pkt->net_ethsrc, ETH_ADDR_LEN );
    msg->net_ethtype = htons ( ETH_TYPE_A );
    return msg;
}


