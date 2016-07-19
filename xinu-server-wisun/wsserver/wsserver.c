/* wsserver.c - Wi-SUN emulation testbed server */

/*--------------------
 * headers
 * ------------------*/
#include <xinu.h>
#include <stdio.h>
#include <string.h>


/*---------------
 * UDP port
 * TIME_OUT value
 * ping status
 * ------------*/
#define PORT 55000
#define TIME_OUT 600000
#define NOTACTIV 0
#define ALIVE    1
#define NOTRESP  -1
#define XOFF     0
#define XON      1

int32 nodeid = 0;
int32 nnodes = 0;
byte ack_info[16];
int ping_ack_flag[46];
int online = 1;
int32 *seqnum;

/*-----------------------------------------------------------------
 * Network topology data strucutres which are used to keep current
 * and old network topology information in the RAM.
 * --------------------------------------------------------------*/

struct t_entry topo[MAXNODES];
struct t_entry old_topo[MAXNODES];

/*---------------------------------------------------------------
 * Initialize topology database with zeros
 * --------------------------------------------------------------*/
void initialize_topo()
{
    int i, j;

    for ( i = 0; i < MAXNODES; i++ ) {
        topo[i].t_nodeid = i;
        topo[i].t_neighbors[0] = 1;

        for ( j = 1; j < 6; j++ )
            topo[i].t_neighbors[j] = 0;

        topo[i].t_status = 0;
    }

    seqnum = ( int32 * ) getmem ( sizeof ( int32 ) );
    *seqnum = 0;

    for ( i = 0; i < 46; i++ )
        ping_ack_flag[i] = 0;
}

/*---------------------------------------------------------------------------------
 * controle message handler is used to call appropiate function based on message
 * which is received from the management server.
 * ----------------------------------------------------------------------------*/
struct c_msg * cmsg_handler ( struct c_msg ctlpkt )
{
    int xonoffid;
    struct c_msg *cmsg_reply;
    int32 mgm_msgtyp = ntohl ( ctlpkt.cmsgtyp );
    cmsg_reply = ( struct c_msg * ) getmem ( sizeof ( struct c_msg ) );
    memset ( cmsg_reply, 0, sizeof ( struct c_msg ) );

    /*------------------------------------------------------
     * The type of control message should be checked here
     * and then an appropiate response message should be created
     * returned to the caller.
     * ----------------------------------------------------*/
    switch ( mgm_msgtyp ) {
        case C_RESTART:
            kprintf ( "Message type is %d\n", C_RESTART );
            //cmsg_reply->cmsgtyp = htonl(C_OK);
            break;

        case C_RESTART_NODES:
            kprintf ( "Message type is %d\n", C_RESTART_NODES );

            if ( online )
                cmsg_reply->cmsgtyp = htonl ( C_OK );

            break;

        case C_PING_REQ:
            kprintf ( "Message type is %d\n", C_PING_REQ );

            if ( online )
                cmsg_reply = nping_reply ( ctlpkt );

            break;

        case C_PING_ALL:
            kprintf ( "Message type is %d\n", C_PING_ALL );

            if ( online )
                cmsg_reply = nping_all_reply ( ctlpkt );

            break;

        case C_XOFF:
            kprintf ( "Message type is %d\n", C_XOFF );

            if ( online ) {
                xonoffid = ntohl (ctlpkt.xonoffid);

                if (wsserver_xonoff (XOFF, xonoffid) == OK)
                    cmsg_reply->cmsgtyp = htonl ( C_OK );

                else
                    cmsg_reply->cmsgtyp = htonl (C_ERR);
            }

            break;

        case C_XON:
            kprintf ( "Message type is %d\n", C_XON );

            if ( online ) {
                xonoffid = ntohl (ctlpkt.xonoffid);

                if (wsserver_xonoff (XON, xonoffid) == OK)
                    cmsg_reply->cmsgtyp = htonl ( C_OK );

                else
                    cmsg_reply->cmsgtyp = htonl (C_ERR);
            }

            break;

        case C_OFFLINE:
            online = 0;
            kprintf ( "Message type is %d\n", C_OFFLINE );
            cmsg_reply->cmsgtyp = htonl ( C_OK );
            break;

        case C_ONLINE:
            online = 1;
            kprintf ( "Message type is %d\n", C_ONLINE );
            cmsg_reply->cmsgtyp = htonl ( C_OK );
            break;

        case C_TOP_REQ:
            kprintf ( "Message type is %d\n", C_TOP_REQ );
            cmsg_reply = toporeply();
            break;

        case C_NEW_TOP:
            kprintf ( "Message type is %d\n", C_NEW_TOP );
            cmsg_reply = newtop ( ctlpkt );
            break;

        case C_TS_REQ:
            kprintf ( "Message type is %d\n", C_TS_REQ );
            cmsg_reply->cmsgtyp = htonl ( C_TS_RESP );
            break;

        default:
            kprintf ( "ERROR\n" );
            cmsg_reply->cmsgtyp = htonl ( C_ERR );
    }

    return cmsg_reply;
}

/*--------------------------------------------------------------------
 * Create an Ethernet packet and allocate memory
*--------------------------------------------------------------------*/
struct etherPkt *create_etherPkt()
{
    struct etherPkt *msg;
    msg = ( struct etherPkt * ) getmem (sizeof (struct etherPkt));
    //memset(msg, 0, sizeof(msg));
    return msg;
}


/*------------------------------------------------------------------------
*Use the remote file system to open and read a topology database file
--------------------------------------------------------------------------*/
status init_topo ( char *filename )
{
    char *buff;
    uint32 size;
    int status = read_topology ( filename, &buff, &size );

    if ( status == SYSERR ) {
        kprintf ( "WARNING: Could not read topology file\n" );
        return SYSERR;
    }

    nnodes = topo_update ( buff, size, topo );
    freemem ( buff, size );
    return OK;
}
/* -----------------------------------------------------------------
 * Thid function is used to update MAC address field for
 * a node in the topology database which is kept in RAM.
 * ----------------------------------------------------------------*/
void topo_update_mac ( struct netpacket *pkt )
{
    topo[nodeid].t_status = 1;
    //int i;
    memcpy ( topo[nodeid].t_macaddr, pkt->net_ethsrc, ETH_ADDR_LEN );
    /*for (i=0; i<6; i++) {
    kprintf("%02x:", topo[nodeid].t_macaddr[i]);
     }
    kprintf("\n");*/
    freebuf ( ( char * ) pkt );
    wsserver_xonoff (XON, nodeid);
    nodeid++;
}

/*---------------------------------------------------------
 * Print old and new topology database
 * ------------------------------------------------------*/
void print_topo()
{
    int i, j;

    for ( i = 0; i < MAXNODES; i++ ) {
        kprintf ( "node id: %d : %d -- status: %d : %d -- mcastaddr:  ", old_topo[i].t_nodeid , topo[i].t_nodeid, old_topo[i].t_status, topo[i].t_status );

        for ( j = 0; j < 6; j++ )
            kprintf ( "%d ", old_topo[i].t_neighbors[j] );

        kprintf ( "  :  " );

        for ( j = 0; j < 6; j++ )
            kprintf ( "%d ", topo[i].t_neighbors[j] );

        kprintf ( "\n  -- macaddr:  " );

        for ( j = 0; j < 6; j++ )
            kprintf ( "%02x ", old_topo[i].t_macaddr[j] );

        kprintf ( "  :  " );

        for ( j = 0; j < 6; j++ )
            kprintf ( "%02x ", topo[i].t_macaddr[j] );

        kprintf ( "\n" );
    }
}

/*---------------------------------------------------------
 * comparisson old and new topology and send assign messages
 * to the nodes that should receive a new multicast address
 * ------------------------------------------------------*/
status topo_compr()
{
    int i, j , k;
    int flag = 0;
    struct netpacket *pkt;
    pkt = ( struct netpacket * ) getbuf ( netbufpool );

    //memset(&pkt, 0, sizeof(struct netpacket));
    //kprintf("number of nodes: %d\n", nnodes);
    for ( i = 0; i < nnodes; i++ ) {
        //kprintf("nodeid , i: %d, %d\n", nodeid, i);
        flag = 0;

        for ( j = 0; j < 6; j++ )
            if ( old_topo[i].t_neighbors[j] == topo[i].t_neighbors[j] )
                flag++;

        if ( nodeid < i ) {
            freebuf ( ( char * ) pkt );
	   kprintf("nodeid<i %d:%d\n",nodeid, i);
            return SYSERR;
        }

        if ( flag == 6 ) {
            topo[i].t_status = 1;
            //kprintf("mcast addresses are the same\n");
            wsserver_xonoff (XON, i);
            nodeid += 1;

        } else {
            //kprintf("nnodes , i: %d, %d\n", nnodes, i);
            for ( k = 0; k < 6; k++ ) {
                //kprintf("%02x ",topo[i].t_macaddr[k]);
                pkt->net_ethsrc[k] = topo[i].t_macaddr[k];
            }

            //kprintf("\n");
            //memcpy(&pkt->net_ethsrc, topo[i].t_macaddr, ETH_ADDR_LEN);
            if ( wsserver_assign ( pkt ) == OK ) {
                sleepms (20);

            } else {
                freebuf ( ( char * ) pkt );
		kprintf("assign error\n");
                return SYSERR;
            }
        }
    }

    for (i = nnodes; i < 46; i++) {
        if (old_topo[i].t_status == 1) {
            wsserver_xonoff (XOFF, i);
        }
    }

    freebuf ( ( char * ) pkt );
    return OK;
}
/*----------------------------------------------------------
* This function is used to update the network topology
 * information.
 * ----------------------------------------------------------*/
struct c_msg *newtop ( struct c_msg ctlpkt )
{
    char *fname;
    int nnodes_temp;
    status stat;
    struct c_msg *cmsg_reply;
    cmsg_reply = ( struct c_msg * ) getmem ( sizeof ( struct c_msg ) );
    memset ( cmsg_reply, 0, sizeof ( struct c_msg ) );
    memcpy ( &old_topo, &topo, sizeof ( topo ) );
    fname = getmem ( sizeof ( ctlpkt.fname ) );
    strcpy ( fname, ( char * ) ctlpkt.fname );
    nnodes_temp = nnodes;
    stat = init_topo ( fname );
    //kprintf("stat:%d\n", stat);
    //print_topo();
    if ( stat == OK ) {
	nodeid = 0;
        if ( topo_compr() == OK ) {
            cmsg_reply->cmsgtyp = htonl ( C_OK );

        } else {
            memcpy ( &topo, &old_topo, sizeof ( topo ) );
	    nnodes = nnodes_temp;
            cmsg_reply->cmsgtyp = htonl ( C_ERR );
	   
        }

    } else {
        memcpy ( &topo, &old_topo, sizeof ( topo ) );
	nnodes = nnodes_temp;
        cmsg_reply->cmsgtyp = htonl ( C_ERR );
	
    }

    return cmsg_reply;
}

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
/*-----------------------------------------------------------------
 *  Make the PING REPLY message
 *  -------------------------------------------------------------*/
struct c_msg * nping_reply ( struct c_msg ctlpkt )
{
    int32 i;
    struct c_msg * cmsg_reply;
    cmsg_reply = ( struct c_msg * ) getmem ( sizeof ( struct c_msg ) );
    memset ( cmsg_reply, 0, sizeof ( struct c_msg ) );
    int ping_num = 0;
    i = ntohl ( ctlpkt.pingnodeid );

    if ( topo[i].t_status == 1 ) {
        status stat = nping ( i );
        sleepms (20);

        if ( ping_ack_flag[i] == 1 && stat == OK ) {
            cmsg_reply->pingdata[ping_num].pnodeid = htonl ( i );
            cmsg_reply->pingdata[ping_num].pstatus = htonl ( ALIVE );
            ping_ack_flag[i] = 0;

        } else if ( ping_ack_flag[i] == 0 && stat == OK ) {
            cmsg_reply->pingdata[ping_num].pnodeid = htonl ( i );
            cmsg_reply->pingdata[ping_num].pstatus = htonl ( NOTRESP );
        }

    } else {
        cmsg_reply->pingdata[ping_num].pnodeid = htonl ( i );
        cmsg_reply->pingdata[ping_num].pstatus = htonl ( NOTACTIV );
    }

    ping_num = 1;
    cmsg_reply->cmsgtyp = htonl ( C_PING_REPLY );
    cmsg_reply->pingnum = htonl ( ping_num );
    return cmsg_reply;
}
/*------------------------------------------------------------------
 * This function is used to ping a speceifc node
 * which is determined by the mgmt app
 * ----------------------------------------------------------------*/
status nping ( int32 pingnodeid )
{
    struct etherPkt *ping_msg;
    byte *seq_buf;
    seq_buf = ( byte * ) getmem ( sizeof ( byte ) * 4 );
    ping_msg = create_etherPkt();
    int32 retval;
    /* fill out the Ethernet packet fields */
    memcpy ( ping_msg->src, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( ping_msg->dst, topo[pingnodeid].t_macaddr, ETH_ADDR_LEN );
    //kprintf("node ID is: %d ,node mac address: ", pingnodeid);
    /*for (i = 0; i<6; i++)
    {
        //  	    kprintf("%02x:", topo[pingnodeid].t_macaddr[i]);
    }*/
    //kprintf("\n");
    ping_msg->type = htons ( ETH_TYPE_A );
    ping_msg->msg.amsgtyp = htonl ( A_PING ); /* Error message */
    seq_buf = ( byte * ) seqnum;
    //kprintf("seq num: %d, seq buf: %d\n", *seqnum , *seq_buf);
    memcpy ( ping_msg->msg.apingdata , seq_buf , sizeof ( byte ) * 4 );
    *seqnum = *seqnum + 1;
    memset ( ack_info, 0, sizeof ( ack_info ) );
    memcpy ( ack_info, ( char * ) ( ping_msg ) + 14, 16 );
    /*send packet over Ethernet */
    retval = write ( ETHER0, ( char * ) ping_msg, sizeof ( struct etherPkt ) );
    freemem ( ( char * ) ping_msg, sizeof ( struct etherPkt ) );

    if ( retval > 0 ) {
        return OK;

    } else
        return SYSERR;
}

/*-----------------------------------------------------------------
 *  Make the PING ALL REPLY message
 *  -------------------------------------------------------------*/
struct c_msg * nping_all_reply ( struct c_msg ctlpkt )
{
    int32 i;
    struct c_msg * cmsg_reply;
    cmsg_reply = ( struct c_msg * ) getmem ( sizeof ( struct c_msg ) );
    memset ( cmsg_reply, 0, sizeof ( struct c_msg ) );
    int ping_num = 0;
    status stat = nping_all();

    if ( stat == OK ) {
	    kprintf("nnodes:%d\n", nnodes);
	    if (nnodes != 0)
		    sleepms( nnodes + 20);
	    else
		    sleepms(MAXNODES + 20);

        for ( i = 0; i < MAXNODES; i++ ) {
            if ( topo[i].t_status == 1 ) {
                //kprintf("i:ping ack flag  %d:%d\n", i,  ping_ack_flag[i]);
                if ( ping_ack_flag[i] == 1 ) {
                    cmsg_reply->pingdata[ping_num].pnodeid = htonl ( i );
                    cmsg_reply->pingdata[ping_num].pstatus = htonl ( ALIVE );
                    ping_ack_flag[i] = 0;

                } else if ( ping_ack_flag[i] == 0 ) {
                    cmsg_reply->pingdata[ping_num].pnodeid = htonl ( i );
                    cmsg_reply->pingdata[ping_num].pstatus = htonl ( NOTRESP );
                }
            }

            ping_num++;
            //kprintf("pingnum: %d\n",ping_num);
        }

        cmsg_reply->cmsgtyp = htonl ( C_PING_ALL );
        cmsg_reply->pingnum = htonl ( ping_num );
    }

    return cmsg_reply;
}
/*------------------------------------------------------------------
 * This function is used to send a broadcast ping message to all the
 * nodes in active topology
 * ----------------------------------------------------------------*/
status nping_all()
{
    struct etherPkt *ping_msg;
    byte *seq_buf;
    seq_buf = ( byte * ) getmem ( sizeof ( byte ) * 4 );
    ping_msg = create_etherPkt();
    int32 retval;
    /* fill out the Ethernet packet fields */
    memcpy ( ping_msg->src, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( ping_msg->dst, NetData.ethbcast, ETH_ADDR_LEN );
    ping_msg->type = htons ( ETH_TYPE_A );
    ping_msg->msg.amsgtyp = htonl ( A_PING_ALL ); /* Error message */
    seq_buf = ( byte * ) seqnum;
    //kprintf("seq num: %d, seq buf: %d\n", *seqnum , *seq_buf);
    memcpy ( ping_msg->msg.apingdata , seq_buf , sizeof ( byte ) * 4 );
    *seqnum = *seqnum + 1;
    memset ( ack_info, 0, sizeof ( ack_info ) );
    memcpy ( ack_info, ( char * ) ( ping_msg ) + 14, 16 );
    /*send packet over Ethernet */
    retval = write ( ETHER0, ( char * ) ping_msg, sizeof ( struct etherPkt ) );
    freemem ( ( char * ) ping_msg, sizeof ( struct etherPkt ) );

    if ( retval > 0 )
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
    topo_reply = ( struct c_msg * ) getmem ( sizeof ( struct c_msg ) );
    int32 i, j;
    topo_reply->cmsgtyp = htonl ( C_TOP_REPLY );
    topo_reply->topnum = htonl ( nnodes );

    for ( i = 0; i < nnodes; i++ ) {
        topo_reply->topdata[i].t_nodeid = htonl ( topo[i].t_nodeid );
        topo_reply->topdata[i].t_status = htonl ( topo[i].t_status );

        for ( j = 0; j < 6; j++ ) {
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
    struct c_msg *replypkt;
    printf ( "=========== Started Wi-SUN testbed server ===========\n" );
    /* Register UDP port for use by server */
    slot = udp_register ( 0, 0, serverport );

    if ( slot == SYSERR ) {
        kprintf ( "testbed server could not get UDP port %d\n", serverport );
        return 1;
    }

    while ( TRUE ) {
        /* Attempt to receive control packet */
        retval = udp_recvaddr ( slot, &remip, &remport, ( char * ) &ctlpkt,
                                sizeof ( ctlpkt ), timeout );

        if ( retval == TIMEOUT ) {
            continue;

        } else if ( retval == SYSERR ) {
            kprintf ( "WARNING: UDP receive error in testbed server\n" );
            exit();

        } else {
            int32 mgm_msgtyp = ntohl ( ctlpkt.cmsgtyp );
            kprintf ( "*====> Got control message %d\n", mgm_msgtyp );
            replypkt = cmsg_handler ( ctlpkt );
            status sndval = udp_sendto ( slot, remip, remport,
                                         ( char * ) replypkt, sizeof ( struct c_msg ) );

            //kprintf("send to \n");
            if ( sndval == SYSERR ) {
                kprintf ( "WARNING: UDP send error in testbed server\n" );

            } else {
                //kprintf("here\n");
                int32 reply_msgtyp = ntohl ( replypkt->cmsgtyp );
                kprintf ( "* <==== Replied with %d\n", reply_msgtyp );
            }

            freemem ( ( char * ) replypkt, sizeof ( struct c_msg ) );
            //freemem (ctlpkt, sizeof ( struct c_msg ) );
        }
    }
}



/*-------------------------------------------------------------------------
 * Send error message as a response to a node.
 * --------------------------------------------------------------------------*/
status wsserver_senderr ( struct netpacket *pkt )
{
    struct etherPkt *err_msg;
    err_msg = create_etherPkt();
    int32 retval;
    /* fill out the Ethernet packet fields */
    memcpy ( err_msg->src, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( err_msg->dst, pkt->net_ethsrc, ETH_ADDR_LEN );
    err_msg->type = htons ( ETH_TYPE_A );
    err_msg->msg.amsgtyp = htonl ( A_ERR ); /* Error message */
    /*send packet over Ethernet */
    retval = write ( ETHER0, ( char * ) err_msg, sizeof ( struct etherPkt ) );
    freemem ( ( char * ) err_msg, sizeof ( struct etherPkt ) );
    freebuf ( ( char * ) pkt );

    if ( retval > 0 )
        return OK;

    else
        return SYSERR;
}
/*-----------------------------------------------------------
 * Send an assign message as a response of JOIN message that is
 * received from a node.
 * ----------------------------------------------------------*/
status wsserver_assign ( struct netpacket *pkt )
{

    /*if (nodeid >= 46)
    {
	    freebuf((char *)pkt);
	    return SYSERR;
    }*/
    struct etherPkt *assign_msg;
    assign_msg = create_etherPkt();
    int32 retval;
    int i;
    /* fill out Ethernet packet fields */
    //for (i=0;i < 6;i++)
    //{
    //kprintf("%02x:", pkt->net_ethsrc[i]);
    //}
    memcpy ( assign_msg->src, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( assign_msg->dst, pkt->net_ethsrc, ETH_ADDR_LEN );
    assign_msg->type = htons ( ETH_TYPE_A );
    assign_msg->msg.amsgtyp = htonl ( A_ASSIGN ); /*Assign message */
    assign_msg->msg.anodeid = htonl ( nodeid );

    //kprintf("Assign type: %d:%d\n", htonl(A_ASSIGN), htonl(nodeid));
    for ( i = 0; i < 6; i++ ) {
        assign_msg->msg.amcastaddr[i] = topo[nodeid].t_neighbors[i];
        //kprintf("%02x:", assign_msg->amcastaddr[i]);
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
	//kprintf("here\n");
        return SYSERR;
    }
}

/*----------------------------------------------------------------
 *  ACK handler
 *  -------------------------------------------------------------*/
void ack_handler ( struct netpacket *pkt )
{
    struct etherPkt *node_msg;
    node_msg = ( struct etherPkt * ) pkt;
    int i, ack;
    ack = 0;

    for ( i = 0; i < 16; i++ ) {
        if ( node_msg->msg.aacking[i] == ack_info[i] ) {
            ack++;
        }

        //kprintf("aacking:%d:%d\n", node_msg->aacking[i],ack_info[i]);
    }

    i = ntohl ( node_msg->msg.anodeid );

    if ( ack == 16 ) {
        if ( ack_info[3] == A_ASSIGN ) {
            topo_update_mac ( pkt );
            kprintf ( "====>Assign ACK message is received\n" );

        } else if ( ack_info[3] == A_PING ) {
            
            ping_ack_flag[i] = 1;
            freebuf ( ( char * ) pkt );
            kprintf("--->Ping ACK message is received\n");

        } else if ( ack_info[3] == A_PING_ALL ) {
            ping_ack_flag[i] = 1;
            freebuf ( ( char * ) pkt );
        }

    } else
        freebuf ( ( char * ) pkt );
}

/*-----------------------------------------------------------------
 * message handler is used to call appropiate function based
 * on a message which is received from a node.
 * ----------------------------------------------------------------*/

void  amsg_handler ( struct netpacket *pkt )
{
    struct etherPkt *node_msg;
    status retval;
    node_msg = ( struct etherPkt * ) pkt;
    /* -------------------------------------------
     * Extract message type for TYPE A frames
     * ------------------------------------------*/
    int32 amsgtyp = ntohl ( node_msg->msg.amsgtyp );

    //kprintf("type:%d\n", amsgtyp);
    switch ( amsgtyp ) {
        case A_JOIN:
            kprintf ( "====>Join message is received\n" );

            if ( online ) {
                retval = wsserver_assign ( pkt );

                if ( retval == OK ) {
                    kprintf ( "<====Assign message is sent\n" );
                }

            } else
                freebuf ( ( char * ) pkt );

            break;

        case A_ACK:
            ack_handler ( pkt );
            break;

        case A_ERR:
            wsserver_senderr ( pkt );
            kprintf ( "====>Error message is received\n" );
            break;

        default:
            freebuf ( ( char * ) pkt );
            break;
    }

    return ;
}


