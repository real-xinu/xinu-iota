/* wsserver.c - Wi-SUN emulation testbed server */

#include <xinu.h>
#include <stdio.h>
struct node_info
{
    int32 nodeid;
    byte mcastaddr[6];
};

struct node_info info;
/*-----------------------------------------------------------
* Create and Ethernet packet and fill out header fields
*----------------------------------------------------------*/
struct etherPkt *create_etherPkt(struct netpacket *pkt)
{
    struct etherPkt *msg;
    msg= (struct etherPkt *)getmem(sizeof(struct etherPkt));

    /*fill out Ethernet packet header fields */
    memset(msg, 0, sizeof(msg));
    memcpy(msg->src, NetData.ethucast, ETH_ADDR_LEN);
    memcpy(msg->dst, pkt->net_ethsrc, ETH_ADDR_LEN);
    msg->type = htons(ETH_TYPE_A);
    return msg;



}
/*------------------------------------------------
 * Send a join broadcast message when the node boots up.
 * -------------------------------------------------*/
status wsnode_join()
{
    struct etherPkt *join_msg;
    int32 retval;
    join_msg= (struct etherPkt *)getmem(sizeof(struct etherPkt));

    /*fill out Ethernet packet header fields */
    memset(join_msg, 0, sizeof(join_msg));
    memcpy(join_msg->src, NetData.ethucast, ETH_ADDR_LEN);
    memcpy(join_msg->dst, NetData.ethbcast, ETH_ADDR_LEN);
    join_msg->type = htons(ETH_TYPE_A);

    /*fill out Ethernet packet data fields */
    join_msg->amsgtyp = htonl(A_JOIN);
    join_msg->anodeid = htons(0);
    retval = write(ETHER0, (char *)join_msg, sizeof(struct etherPkt));

    if(retval > 0)
        return OK;
    else
        return SYSERR;

}
/*-------------------------------------------------------------
 * Send ack message as a repsonse of assign and ping messages
 * ----------------------------------------------------------*/
status wsnode_sendack(struct netpacket *pkt)
{
    struct etherPkt *ack_msg;
    ack_msg = create_etherPkt(pkt);
    int32 retval;

    ack_msg->amsgtyp = htonl(A_ACK);
    retval = write(ETHER0, (char *)ack_msg, sizeof(struct etherPkt));
    if(retval > 0)
        return OK;
    else
        return SYSERR;

}
/*------------------------------------------------
 * Message handler is used to call
 * appropiate function based on message type.
 * -----------------------------------------------*/
void amsg_handler(struct netpacket *pkt)
{
    int i;
    struct etherPkt *node_msg;
    node_msg = (struct etherPkt *)pkt;
    int32 amsgtyp = ntohl(node_msg->amsgtyp);
    switch(amsgtyp)
    {
    case A_ASSIGN:
	pdump(pkt);
        if(wsnode_sendack(pkt)== OK)
            kprintf("--->ACK message is sent\n");
        break;
    case A_RESTART:
        kprintf("<--- RESTART message is received\n");
        break;
    case A_XOFF:
        kprintf("<---XOF message is received\n");
        break;
    case A_XON:
        kprintf("<--- XON message is received\n");
        break;
    case A_PING:
        kprintf("<--- PING message is received\n");
        break;
    case A_PING_ALL:
        kprintf("<--- PINGALL message is received\n");
        break;

    }


}


