/* wsserver.c - Wi-SUN emulation testbed server */

#include <xinu.h>
#include <stdio.h>

int32 nodeid = 0;
int32 nnodes = 0;

#define PORT 55000  /* UDP PORT */
#define TIME_OUT 600000 

/*-----------------------------------------------------------------
 * Network topology data strucutre which is used to keep current 
 * network topology information in RAM.
 * --------------------------------------------------------------*/

struct t_entry topo[MAXNODES];


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
        cmsg_reply = toporeply();
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

/*-----------------------------------------------------------------------
*Use the remote file system to open and read a topology database file named Top.0
-------------------------------------------------------------------------*/
status init_topo(char *filename)
{

    char *buff;
    uint32 size;
    int status = read_topology(filename, &buff, &size);
    if (status == SYSERR)
    {
        kprintf("WARNING: Could not read topology file\n");
        
    }

    nnodes = topo_update(buff, size, topo);
    freemem(buff, size);

    return OK;

}

/*-------------------------------------------------------------------
 * Create a topo reply message as a response of topo request message 
 * --------------------------------------------------------------------*/
struct c_msg *toporeply()
{
    struct c_msg *topo_reply;
    topo_reply = (struct c_msg *)getmem(sizeof(struct c_msg));
    int32 i, j;

    topo_reply->cmsgtyp = htonl(C_TOP_REPLY);
    topo_reply->topnum = htonl(nnodes);
    for (i=0; i<nnodes; i++)
    {
        topo_reply->topdata[i].t_nodeid = htonl(topo[i].t_nodeid);
        topo_reply->topdata[i].t_status = htonl(topo[i].t_status);

        for (j=0; j<6; j++)
        {
            topo_reply->topdata[i].t_neighbors[j] = topo[i].t_neighbors[j];

        }

    }

    return topo_reply;
}


/*------------------------------------------------------------------------
 * wsserver  -  Server to manage the Wi-SUN emulation testbed
 *------------------------------------------------------------------------
 */
process	wsserver ()
{
    uint16 serverport = PORT;
    int32 slot;
    int32 retval;
    uint32 remip;
    uint16 remport;
    uint32 timeout = TIME_OUT;
    struct c_msg ctlpkt;

    printf("=== Started Wi-SUN testbed server ===\n");

    /* Register UDP port for use by server */
    slot = udp_register(0, 0, serverport);
    if (slot == SYSERR)
    {
        kprintf("testbed server could not get UDP port %d\n", serverport);
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
            kprintf("WARNING: UDP receive error in testbed server\n");
            continue; /* may be better to have the server terminate? */
        }
        else
        {
            int32 mgm_msgtyp = ntohl(ctlpkt.cmsgtyp);
            kprintf("* => Got control message %d\n", mgm_msgtyp);

            struct c_msg *replypkt = cmsg_handler(mgm_msgtyp);

            status sndval = udp_sendto(slot, remip, remport,
                                       (char *) replypkt, sizeof(struct c_msg));
            if (sndval == SYSERR)
            {
                kprintf("WARNING: UDP send error in testbed server\n");
            }
            else
            {
                int32 reply_msgtyp = ntohl(replypkt->cmsgtyp);
                kprintf("* <= Replied with %d\n", reply_msgtyp);
            }

            freemem((char *) replypkt, sizeof(struct c_msg));
        }
    }
}



/*-------------------------------------------------------------------------
 * Send error message as a response to a node.
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
    int i;
    /* fill out Ethernet packet fields */

    memcpy(assign_msg->src, NetData.ethucast, ETH_ADDR_LEN);
    memcpy(assign_msg->dst, pkt->net_ethsrc, ETH_ADDR_LEN);
    assign_msg->type = htons(ETH_TYPE_A);
    assign_msg->amsgtyp = htonl(A_ASSIGN);  /*Assign message */
    assign_msg->anodeid = htonl(nodeid);
    for (i=0; i<6; i++)
    {
	    assign_msg->amcastaddr[i] = topo[nodeid].t_neighbors[i];
    }
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
	{
            kprintf("<-Assign message is sent\n");
	    nodeid++;
	}
        break;

    case A_ACK:
        kprintf("--->ACK message is recevied\n");
        break;
    case A_ERR:
        kprintf("--->Error message is received\n");
        break;
    default:
        wsserver_senderr(pkt);	

    }

}

