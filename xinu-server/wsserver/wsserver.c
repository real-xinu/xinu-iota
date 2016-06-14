/* wsserver.c - Wi-SUN emulation testbed server */

#include <xinu.h>
#include <stdio.h>

int32 nodeid = 0;
/*-----------------------------------------------------------
 * controle message handler is used to
 * call appropiate function based on message
 * which is received from the management server.
 * ---------------------------------------------------------*/
struct c_msg * cmsg_handler(int32 mgm_msgtyp)
{
    struct c_msg *cmsg_reply;
    cmsg_reply = (struct c_msg *) getmem(sizeof(struct c_msg));
    memset(cmsg_reply, 0, sizeof(struct c_msg));

    switch(mgm_msgtyp)
    {
    case C_RESTART:
        printf("Message type is %d\n", C_RESTART);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_RESTART_NODES:
        printf("Message type is %d\n", C_RESTART_NODES);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_PING_REQ:
        printf("Message type is %d\n", C_PING_REQ);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_PING_ALL:
        printf("Message type is %d\n", C_PING_ALL);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_XOFF:
        printf("Message type is %d\n", C_XOFF);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_XON:
        printf("Message type is %d\n", C_XON);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_OFFLINE:
        printf("Message type is %d\n", C_OFFLINE);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_ONLINE:
        printf("Message type is %d\n", C_ONLINE);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_TOP_REQ:
        printf("Message type is %d\n", C_TOP_REQ);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_NEW_TOP:
        printf("Message type is %d\n", C_NEW_TOP);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_TS_REQ:
        printf("Message type is %d\n", C_TS_REQ);
        cmsg_reply->cmsgtyp = htonl(C_TS_RESP);
        break;
    }

    return cmsg_reply;
}

/*--------------------------------------------------------------------
 * Create an Ethernet packet
*--------------------------------------------------------------------*/
struct etherPkt *create_etherPkt()
{
    struct etherPkt *msg;
    msg = (struct etherPkt *)getmem(sizeof(struct etherPkt));
    memset(msg, 0, sizeof(msg));

    return msg;
}

/* TEST FUNCTION for reading and printing a topology */
/* This is meant to show how the functions in wstopology.c can be used */
void dump_topology_test()
{
    char *filename = "Topology-K10.0";

    char *buff;
    int nodes;
    int status = read_topology(filename, &buff, &nodes);
    if (status == SYSERR) {
        printf("WARNING: Could not read topology file\n");
        return;
    }

    printf("~> Raw topology info for %d nodes:\n", nodes);
    print_raw_topology(buff, nodes);
    freemem(buff, nodes * ETH_ADDR_LEN);
}
    

/*------------------------------------------------------------------------
 * wsserver  -  Server to manage the Wi-SUN emulation testbed
 *------------------------------------------------------------------------
 */
process	wsserver ()
{
    uint16 serverport = 55000;
    int32 slot;
    int32 retval;
    uint32 remip;
    uint16 remport;
    uint32 timeout = 600000;
    struct c_msg ctlpkt;

    printf("=== Started Wi-SUN testbed server ===\n");

    dump_topology_test();

    /* Register UDP port for use by server */
    slot = udp_register(0, 0, serverport);
    if (slot == SYSERR)
    {
        printf("testbed server could not get UDP port %d\n", serverport);
        return 1;
    }

    while (TRUE)
    {
        /* Attempt to receive control packet */
        retval = udp_recvaddr(slot, &remip, &remport, (char *) &ctlpkt,
                              sizeof(ctlpkt), timeout);

        if (retval == TIMEOUT)
        {
            continue;
        }
        else if (retval == SYSERR)
        {
            printf("WARNING: UDP receive error in testbed server\n");
            continue; /* may be better to have the server terminate? */
        }
        else
        {
            int32 mgm_msgtyp = ntohl(ctlpkt.cmsgtyp);
            printf("* => Got control message %d\n", mgm_msgtyp);

            struct c_msg *replypkt = cmsg_handler(mgm_msgtyp);

            status sndval = udp_sendto(slot, remip, remport,
                                       (char *) replypkt, sizeof(struct c_msg));
            if (sndval == SYSERR)
            {
                printf("WARNING: UDP send error in testbed server\n");
            }
            else
            {
                int32 reply_msgtyp = ntohl(replypkt->cmsgtyp);
                printf("* <= Replied with %d\n", reply_msgtyp);
            }

            freemem((char *) replypkt, sizeof(struct c_msg));
        }
    }
}

/*------------------------------------------------------------------------
 * Send a broadcast message to nodes and wait to receive join messages
 * from them.
 * -----------------------------------------------------------------------*/
status wsserver_init()
{
    struct etherPkt *msg;
    int32 retval;

    msg = create_etherPkt();
    /* fill out Ethernet packet fields */
    memcpy(msg->src, NetData.ethucast, ETH_ADDR_LEN);
    memcpy(msg->dst, NetData.ethbcast, ETH_ADDR_LEN);
    msg->type = htons(ETH_TYPE_A);
    msg->amsgtyp = htonl(A_RESTART);  /*restart message */

    /*send packet over Ethernet */

    retval = write(ETHER0, (char *)msg, sizeof(struct etherPkt));
    if(retval >0)
        return OK;

    else
        return SYSERR;

}

/*-------------------------------------------------------------------------
 * Send error message as a response to a node.
 *
 * --------------------------------------------------------------------------*/
status wsserver_senderr(struct netpacket *pkt)
{
    struct etherPkt *err_msg;
    err_msg = create_etherPkt();
    int32 retval;

    /* fill out the Ethernet packet fields */
    memcpy(err_msg->src, NetData.ethucast, ETH_ADDR_LEN);
    memcpy(err_msg->dst, pkt->net_ethsrc, ETH_ADDR_LEN);
    err_msg->type = htons(ETH_TYPE_A);
    err_msg->amsgtyp = htonl(A_ERR); /* Error message */

    /*send packet over Ethernet */
    retval = write(ETHER0, (char *)err_msg, sizeof(struct etherPkt));
    if (retval > 0)
        return OK;
    else
        return SYSERR;



}
/*-----------------------------------------------------------
 * Send assign message as a response of JOIN message from a
 * node.
 * ----------------------------------------------------------*/

status wsserver_assign(struct netpacket *pkt)
{
    struct etherPkt *assign_msg;
    assign_msg = create_etherPkt();
    int32 retval;
    /* fill out Ethernet packet fields */

    memcpy(assign_msg->src, NetData.ethucast, ETH_ADDR_LEN);
    memcpy(assign_msg->dst, pkt->net_ethsrc, ETH_ADDR_LEN);
    assign_msg->type = htons(ETH_TYPE_A);
    assign_msg->amsgtyp = htonl(A_ASSIGN);  /*Assign message */
    assign_msg->anodeid = htonl(nodeid);

    retval = write(ETHER0, (char *)assign_msg, sizeof(struct etherPkt));
    if(retval > 0)
        return OK;
    else
        return SYSERR;

}
/*-----------------------------------------------------------------
 * message handler is used to call appropiate function based
 * on a message which is received from a node.
 * ----------------------------------------------------------------*/

void amsg_handler(struct netpacket *pkt)
{
    struct etherPkt *node_msg;
    status retval;
    node_msg = (struct etherPkt *)pkt;
    int32 amsgtyp = ntohl(node_msg->amsgtyp);
    switch(amsgtyp)
    {
    case A_JOIN:
        kprintf("--->Join message is received\n");
        retval = wsserver_assign(pkt);
        if (retval == OK)
            kprintf("<-Assign message is sent\n");
        break;

    case A_ACK:
        kprintf("--->ACK message is recevied\n");
        break;
    case A_ERR:
	kprintf("--->Error message is received\n");
	break;

    }

}

