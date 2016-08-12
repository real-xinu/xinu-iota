#include <xinu.h>

/*-----------------------------------------------------------------
 *  Make the PING REPLY message
 *  -------------------------------------------------------------*/
struct c_msg * nping_reply ( struct c_msg *ctlpkt )
{
    int32 i;
    struct c_msg * cmsg_reply;
    cmsg_reply = ( struct c_msg * ) getmem ( sizeof ( struct c_msg ) );
    memset ( cmsg_reply, 0, sizeof ( struct c_msg ) );
    int ping_num = 0;
    i = ntohl ( ctlpkt->pingnodeid );

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
    /*DEBUG */ //kprintf("node ID is: %d ,node mac address: ", pingnodeid);
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
struct c_msg * nping_all_reply ( struct c_msg *ctlpkt )
{
    int32 i;
    struct c_msg * cmsg_reply;
    cmsg_reply = ( struct c_msg * ) getmem ( sizeof ( struct c_msg ) );
    memset ( cmsg_reply, 0, sizeof ( struct c_msg ) );
    int ping_num = 0;
    status stat = nping_all();

    if ( stat == OK ) {
        kprintf ("nnodes:%d\n", nnodes);

        if (nnodes != 0)
            sleepms ( nnodes + 20);

        else
            sleepms (MAXNODES + 20);

        for ( i = 0; i < MAXNODES; i++ ) {
            if ( topo[i].t_status == 1 ) {
                /*DEBUG */  //kprintf("i:ping ack flag  %d:%d\n", i,  ping_ack_flag[i]);
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
            /*DEBUG */  //kprintf("pingnum: %d\n",ping_num);
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
    /*DEBUG*/ //kprintf("seq num: %d, seq buf: %d\n", *seqnum , *seq_buf);
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


