/* nping.c - nping_reply */

#include <xinu.h>

/*------------------------------------------------------------------------
 * nping_in  -  Handle an incoming NPING request
 *------------------------------------------------------------------------
 */
struct	c_msg *nping_in (
		struct	c_msg *cpkt	/* Incoming control packet 	*/
		)
{
	struct	c_msg *reply;	/* Reply packet			*/
	struct	tbreq *rqptr;	/* Testbed request pointer	*/
	int32	nodeid;		/* Node ID to be pinged		*/
	intmask	mask;		/* Saved interrupt mask		*/
	umsg32	msg;		/* Received message		*/

	/* Allocate memory for reply message */

	reply = (struct c_msg *)getmem(sizeof(struct c_msg));
	if((int32)reply == SYSERR) {
		return SYSERR;
	}

	/* Get the node id */

	nodeid = ntohl(cpkt->pingnodeid);

	/* Initialize common fields */

	reply->cmsgtyp = htonl(C_PING_REPLY);
	reply->pingnum = htonl(1);
	reply->pingdata[0].pnodeid = htonl(nodeid);

	/* If the node is not active, return that status */

	if(topo[nodeid].t_status == 0) {
		reply->pingdata[0].pstatus = htonl(WS_NOTACTIV);
		return reply;
	}

	/* If the queue is full, fail */

	if(semcount(testbed.tsem) >= TB_QSIZE) {
		freebuf((char *)reply);
		return SYSERR;
	}

	/* Insert the request in the testbed queue */

	mask = disable();
	rqptr = &testbed.reqq[testbed.ttail++];
	if(testbed.ttail >= TB_QSIZE) {
		testbed.ttail = 0;
	}

	rqptr->type = TBR_TYPE_NPING;
	memcpy(rqptr->dst, topo[nodeid].t_macaddr, 6);
	rqptr->waiting = TRUE;
	rqptr->waitpid = getpid();
	recvclr();
	restore(mask);

	signal(testbed.tsem);

	/* Wait for a response */

	msg = receive();

	if((int32)msg == SYSERR) {
		reply->pingdata[0].pstatus = htonl(WS_NOTRESP);
	}
	else {
		reply->pingdata[0].pstatus = htonl(WS_ALIVE);
	}

	return reply;
}

/*------------------------------------------------------------------------
 * nping_all_in  -  Handle incoming PING_ALL and PINGALL request
 *------------------------------------------------------------------------
 */
struct	c_msg *nping_all_in (
		struct	c_msg *cpkt,	/* Incoming control packet	*/
		bool8	intopo		/* PING_ALL or PINGALL		*/
		)
{
	kprintf("nping_all_in\n");
    struct	c_msg *reply;	/* Reply packet		*/
	struct	tbreq *rqptr;	/* Testbed request	*/
	umsg32	msg;		/* Received message	*/
	intmask	mask;		/* Saved interrupt mask	*/
	int32	i;		/* Loop index		*/

	reply = (struct tbedpacket *)getmem(sizeof(struct c_msg));
	if((int32)reply == SYSERR) {
		return SYSERR;
	}

	if(intopo) {
		reply->cmsgtyp = htonl(C_PING_ALL);
		for(i = 0; i < nnodes; i++) {
			reply->pingdata[i].pnodeid = htonl(i);
			reply->pingdata[i].pstatus = htonl(WS_NOTRESP);
		}
		reply->pingnum = htonl(nnodes);
	}
	else {
		reply->cmsgtyp = htonl(C_PINGALL_REPLY);
		for(i = 0; i < MAX_BBB; i++) {
			reply->bbb_stat[i] = 0;
		}
		reply->nbbb = htonl(0);
	}

	mask = disable();
	if(semcount(testbed.tsem) >= TB_QSIZE) {
		restore(mask);
		return SYSERR;
	}

	rqptr = &testbed.reqq[testbed.ttail++];
	if(testbed.ttail >= TB_QSIZE) {
		testbed.ttail = 0;
	}

	if(intopo) {
		rqptr->type = TBR_TYPE_NPING_ALL;
	}
	else {
		rqptr->type = TBR_TYPE_NPINGALL;
	}

	rqptr->waiting = TRUE;
	rqptr->waitpid = getpid();
	recvclr();

	restore(mask);

	signal(testbed.tsem);

	while(1) {
		msg = receive();

		kprintf("nping_all_in: msg = %d\n", msg);
		if((int32)msg == SYSERR) {
			break;
		}

		mask = disable();
		if(intopo && topo[msg].t_status) {
			reply->pingdata[msg].pnodeid = htonl(msg);
			reply->pingdata[msg].pstatus = htonl(WS_ALIVE);
		}

		if(!intopo) {
			reply->bbb_stat[msg] = 1;
			reply->nbbb = ntohl(ntohl(reply->nbbb) + 1);
		}
		restore(mask);
	}

	return reply;
}

#if 0
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
            cmsg_reply->pingdata[ping_num].pstatus = htonl ( WS_ALIVE );
            ping_ack_flag[i] = 0;

        } else if ( ping_ack_flag[i] == 0 && stat == OK ) {
            cmsg_reply->pingdata[ping_num].pnodeid = htonl ( i );
            cmsg_reply->pingdata[ping_num].pstatus = htonl ( WS_NOTRESP );
        }

    } else {
        cmsg_reply->pingdata[ping_num].pnodeid = htonl ( i );
        cmsg_reply->pingdata[ping_num].pstatus = htonl ( WS_NOTACTIV );
    }

    ping_num = 1;
    cmsg_reply->cmsgtyp = htonl ( C_PING_REPLY );
    cmsg_reply->pingnum = htonl ( ping_num );
    return cmsg_reply;
}
#endif
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
                    cmsg_reply->pingdata[ping_num].pstatus = htonl ( WS_ALIVE );
                    ping_ack_flag[i] = 0;

                } else if ( ping_ack_flag[i] == 0 ) {
                    cmsg_reply->pingdata[ping_num].pnodeid = htonl ( i );
                    cmsg_reply->pingdata[ping_num].pstatus = htonl ( WS_NOTRESP );
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
 * nodes in current active topology
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

/*-----------------------------------------------------------------
 *  Make the PINGALL REPLY message
 *  -------------------------------------------------------------*/
struct c_msg * pingall_reply ( struct c_msg *ctlpkt )
{
    int32 i;
    struct c_msg * cmsg_reply;
    cmsg_reply = ( struct c_msg * ) getmem ( sizeof ( struct c_msg ) );
    memset ( cmsg_reply, 0, sizeof ( struct c_msg ) );
    int ping_num = 0;
    status stat = pingall();

    if ( stat == OK ) {
     
	sleepms (50);

        for ( i = 0; i < MAX_BBB; i++ ) {
	    cmsg_reply->bbb_stat[i] =  bbb_stat[i];
            if (bbb_stat[i] == 1){	
                    ping_num++;
	    }
        }

        cmsg_reply->cmsgtyp = htonl ( C_PINGALL_REPLY );
        cmsg_reply->nbbb = htonl ( ping_num );
    }
    
    for (i=0; i<MAX_BBB; i++)
	    bbb_stat[i] = 0;
    return cmsg_reply;
}
/*------------------------------------------------------------------
 * This function is used to send a broadcast ping message to all the
 * nodes (active and non active nodes in the current network topology)
 * ----------------------------------------------------------------*/
status pingall()
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
    ping_msg->msg.amsgtyp = htonl ( A_PINGALL ); /* Error message */
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

