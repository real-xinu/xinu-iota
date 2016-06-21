/* wsserver.c - Wi-SUN emulation testbed server */

#include <xinu.h>
#include <stdio.h>
#include <string.h>

int32 nodeid = 0;
int32 nnodes = 0;

#define PORT 55000  /* UDP PORT */
#define TIME_OUT 600000

byte ack_info[16];
int ping_ack_flag = 0;
#define NOTACTIV 0
#define ALIVE    1
#define NOTRESP  -1
/*----------------------------------------------------------------
 * c_msg queue 
 * -------------------------------------------------------------*/
struct c_msg *c_msg_queue;
int queue_index = 0;

struct c_msg dequeue_msg()
{
   int i;
   struct c_msg message;
   message = c_msg_queue[0];
   for (i=1; i<=queue_index; i++)
	   c_msg_queue[i-1] = c_msg_queue[i];
   queue_index--;

   return message;
}
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
struct c_msg * cmsg_handler(struct c_msg ctlpkt)
{

    struct c_msg *cmsg_reply;
    int32 mgm_msgtyp = ntohl(ctlpkt.cmsgtyp);
    cmsg_reply = (struct c_msg *) getmem(sizeof(struct c_msg));
    memset(cmsg_reply, 0, sizeof(struct c_msg));

    switch(mgm_msgtyp)
    {
    case C_RESTART:
        kprintf("Message type is %d\n", C_RESTART);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_RESTART_NODES:
        kprintf("Message type is %d\n", C_RESTART_NODES);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_PING_REQ:
        kprintf("Message type is %d\n", C_PING_REQ);
	cmsg_reply = ping_reply(ctlpkt);
        break;
    case C_PING_ALL:
        kprintf("Message type is %d\n", C_PING_ALL);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_XOFF:
        kprintf("Message type is %d\n", C_XOFF);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_XON:
        kprintf("Message type is %d\n", C_XON);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_OFFLINE:
        kprintf("Message type is %d\n", C_OFFLINE);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_ONLINE:
        kprintf("Message type is %d\n", C_ONLINE);
        cmsg_reply->cmsgtyp = htonl(C_OK);
        break;
    case C_TOP_REQ:
        kprintf("Message type is %d\n", C_TOP_REQ);
        cmsg_reply = toporeply();
        break;
    case C_NEW_TOP:
        kprintf("Message type is %d\n", C_NEW_TOP);
        cmsg_reply = newtop(ctlpkt);
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

/*------------------------------------------------------------------------
*Use the remote file system to open and read a topology database file
(Default file name=Top.0)
--------------------------------------------------------------------------*/
status init_topo(char *filename)
{

    char *buff;
    uint32 size;
    int status = read_topology(filename, &buff, &size);
    if (status == SYSERR)
    {
        kprintf("WARNING: Could not read topology file\n");
        return SYSERR;
    }

    nnodes = topo_update(buff, size, topo);
    freemem(buff, size);

    return OK;

}
/* -----------------------------------------------------------------
 * Thid function is used to update MAC address field in the topology 
 * database.
 * ----------------------------------------------------------------*/
void topo_update_mac(struct netpacket *pkt)
{
    int i;
    topo[nodeid].t_status = 1;
    memcpy(topo[nodeid].t_macaddr, pkt->net_ethsrc, ETH_ADDR_LEN);
    for (i=0;i<6;i++)
    {
	   //kprintf("%02x:", topo[nodeid].t_macaddr[i]);
    }
    //kprintf("\n");
    nodeid++;
}
/*----------------------------------------------------------
* This function is used to update the network topology
 * information.
 * ----------------------------------------------------------*/
struct c_msg *newtop(struct c_msg ctlpkt)
{

    char *fname;
    status stat;
    struct c_msg *cmsg_reply;
    cmsg_reply = (struct c_msg *) getmem(sizeof(struct c_msg));
    memset(cmsg_reply, 0, sizeof(struct c_msg));

    fname = getmem(sizeof(ctlpkt.fname));
    strcpy(fname,(char *)ctlpkt.fname);
    stat = init_topo(fname);
    if (stat == OK)
    {
        cmsg_reply->cmsgtyp = htonl(C_OK);
    }
    else
    {
        cmsg_reply->cmsgtyp = htonl(C_ERR);
    }
    return cmsg_reply;

}
/*-----------------------------------------------------------------
 *  Make the PING REPLY message
 *  -------------------------------------------------------------*/
struct c_msg * ping_reply(struct c_msg ctlpkt)
{
	struct c_msg * cmsg_reply;
        cmsg_reply = (struct c_msg *) getmem(sizeof(struct c_msg));
        memset(cmsg_reply, 0, sizeof(struct c_msg));

	if (topo[ntohl(ctlpkt.pingnodeid)].t_status == 1)
	{
		nping(ctlpkt);
                //sleep(1);
		if (ping_ack_flag == 1)
		{
		 cmsg_reply->cmsgtyp = htonl(C_PING_REPLY);
		 cmsg_reply->pingnum = htonl(1);
		 cmsg_reply->pingdata[0].pnodeid = htonl(ctlpkt.pingnodeid);
		 cmsg_reply->pingdata[0].pstatus = htonl(ALIVE);
                 ping_ack_flag = 0;
		}
		else
		{
		 cmsg_reply->cmsgtyp = htonl(C_PING_REPLY);
		 cmsg_reply->pingnum = htonl(1);
		 cmsg_reply->pingdata[0].pnodeid = htonl(ctlpkt.pingnodeid);
		 cmsg_reply->pingdata[0].pstatus = htonl(NOTRESP);

		}
	}
	else
	{
		 
		 cmsg_reply->cmsgtyp = htonl(C_PING_REPLY);
		 cmsg_reply->pingnum = htonl(1);
		 cmsg_reply->pingdata[0].pnodeid = htonl(ctlpkt.pingnodeid);
		 cmsg_reply->pingdata[0].pstatus = htonl(NOTACTIV);
		
	}
       
	return cmsg_reply;
}
/*------------------------------------------------------------------
 * This function is used to ping a speceifc node 
 * which is determined by the mgmt app 
 * ----------------------------------------------------------------*/
status nping(struct c_msg ctlpkt)
{
    int i;
    struct etherPkt *ping_msg;
    int32 pingnodeid = ntohl(ctlpkt.pingnodeid);
    
    ping_msg = create_etherPkt();
    int32 retval;

    /* fill out the Ethernet packet fields */
    memcpy(ping_msg->src, NetData.ethucast, ETH_ADDR_LEN);
    memcpy(ping_msg->dst, topo[pingnodeid].t_macaddr, ETH_ADDR_LEN);
    //kprintf("node ID is: %d ,node mac address: ", pingnodeid);
    for (i = 0; i<6; i++)
    {
      	    kprintf("%02x:", topo[pingnodeid].t_macaddr[i]);
    }
    kprintf("\n");
    ping_msg->type = htons(ETH_TYPE_A);
    ping_msg->amsgtyp = htonl(A_PING); /* Error message */
     

    memset(ack_info, 0, sizeof(ack_info));
    memcpy(ack_info, (char *)(ping_msg) + 14, 16);


    /*send packet over Ethernet */
    retval = write(ETHER0, (char *)ping_msg, sizeof(struct etherPkt));
    if (retval > 0)
	    return OK;
    else
	    return SYSERR;
}


/*-------------------------------------------------------------------
 * Create a topo reply message as a response of topo request message
 * -----------------------------------------------------------------*/
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

            struct c_msg *replypkt = cmsg_handler(ctlpkt);

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
 * Send an assign message as a response of JOIN message that is
 * received from a node.
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
	kprintf("%02x:", assign_msg->amcastaddr[i]);
    }
    kprintf("\n Assigned Nodeid: %d\n",nodeid);

    memset(ack_info, 0, sizeof(ack_info));
    memcpy(ack_info, (char *)(assign_msg) + 14, 16);

    retval = write(ETHER0, (char *)assign_msg, sizeof(struct etherPkt));
    if(retval > 0)
        return OK;
    else
        return SYSERR;

}

/*----------------------------------------------------------------
 *  ACK handler  
 *  -------------------------------------------------------------*/
void ack_handler(struct netpacket *pkt)
{
	struct etherPkt *node_msg;
	node_msg = (struct etherPkt *)pkt;
	int i, ack;
	ack = 0;
        for (i=0; i<16; i++)
        {
               if (node_msg->aacking[i] == ack_info[i])
	       {
                  ack++;           		       
	       }
               //kprintf("aacking:%d:%d\n", node_msg->aacking[i],ack_info[i]);
        }
	if (ack == 16)
	{
		if (ack_info[5] == A_ASSIGN)
		{
			topo_update_mac(pkt);
			kprintf("--->Assign ACK message is received\n");
		}
		else
		{
			ping_ack_flag = 1;
			kprintf("--->Ping ACK message is received\n");
		}
	}

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
            kprintf("<---Assign message is sent\n");
         
        }
        break;

    case A_ACK:
	ack_handler(pkt);
        break;
    case A_ERR:
        wsserver_senderr(pkt);
        kprintf("--->Error message is received\n");
        break;
        

    }

}


