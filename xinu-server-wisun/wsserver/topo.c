#include <xinu.h>
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

/* -----------------------------------------------------------------
 * Thid function is used to update MAC address field for
 * a node in the topology database which is kept in RAM.
 * ----------------------------------------------------------------*/
void topo_update_mac ( struct netpacket *pkt )
{
    topo[nodeid].t_status = 1;
    //int i;
    memcpy ( topo[nodeid].t_macaddr, pkt->net_ethsrc, ETH_ADDR_LEN );
    /*DEBUG *//*for (i=0; i<6; i++) {
    kprintf("%02x:", topo[nodeid].t_macaddr[i]);
     }
    kprintf("\n");*/
    freebuf ( ( char * ) pkt );
    wsserver_xonoff (XON, nodeid);
    nodeid++;
}

/*---------------------------------------------------------
 * comparision old and new topology and send assign messages
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
            /*DEBUG */ //kprintf ("nodeid<i %d:%d\n", nodeid, i);
            return SYSERR;
        }

        if ( flag == 6 ) {
            topo[i].t_status = 1;
            /*DEBuG *///kprintf("mcast addresses are the same\n");
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
                kprintf ("assign error\n");
                return SYSERR;
            }
        }
    }

    for (i = nnodes; i < MAXNODES; i++) {
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
struct c_msg *newtop ( struct c_msg *ctlpkt )
{
    char *fname;
    int nnodes_temp;
    status stat;
    struct c_msg *cmsg_reply;
    cmsg_reply = ( struct c_msg * ) getmem ( sizeof ( struct c_msg ) );
    memset ( cmsg_reply, 0, sizeof ( struct c_msg ) );
    memcpy ( &old_topo, &topo, sizeof ( topo ) );
    fname = getmem ( sizeof ( ctlpkt->fname ) );
    strcpy ( fname, ( char * ) ctlpkt->fname );
    nnodes_temp = nnodes;
    stat = read_topo ( fname );

    /*DEBUG */ //kprintf("stat:%d\n", stat);

    /*DEBUG *///print_topo();

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





