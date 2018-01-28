/* testbed_assign.c - testbed_assign */

#include <xinu.h>

/*------------------------------------------------------------------------
 * testbed_assign  -  Send an Assign message to a node
 *------------------------------------------------------------------------
 */

int32 get_bbbid(byte macaddr[6])
{
    
    int i;

    for (i = 0; i < MAX_BBB; i++)
    {
       if (!memcmp(macaddr, bbb_macs[i], 6))
          break;
    }

    if ( i >= MAX_BBB)
      return SYSERR;
    else
      return ( i + 101 );
}

/*--------------------------------------------------------
 * -----------------------------------------------------*/


status	testbed_assign (
		int32	index,		/* Index in the topo	*/
		byte	ethaddr[]	/* Ethernet address	*/
		)
{
	struct	tbreq *rqptr;	/* Testbed request	*/
	intmask	mask;		/* Saved interrupt mask	*/
	int32	nodeid;		/* Node ID		*/
	int32	free = -1;	/* Free slot in topo	*/
	int32	i;		/* For loop indexes	*/

	kprintf("testbed_assign: %d %d\n", index, ethaddr);

	mask = disable();

	if(index == -1) {

		/* See, if we already have a mapping */

		for(i = 0; i < MAXNODES; i++) {
			if(topo[i].t_status == 0) {
				free = (free == -1) ? i : free;
				continue;
			}
			if(!memcmp(topo[i].t_macaddr, ethaddr, 6)) {
				break;
			}
		}
		if(i < MAXNODES) {
			index = i;
		}
		else {
			if(free == -1) {
				kprintf("testbed_assign: no free slot in map\n");
				restore(mask);
				return SYSERR;
			}
			index = free;
			topo[free].t_nodeid = testbed.nextid++;
			memcpy(topo[free].t_macaddr, ethaddr, 6);
			topo[free].t_bbbid = get_bbbid(topo[free].t_macaddr);
			topo[free].t_status = 1;
		}
	}

	/* If no more space in testbed queue, fail */

	if(semcount(testbed.tsem) >= TB_QSIZE) {
		kprintf("testbed_assign: testbed reqeust q full\n");
		restore(mask);
		return SYSERR;
	}

	/* Insert a request in the testbed request queue */

	rqptr = &testbed.reqq[testbed.ttail++];
	if(testbed.ttail >= TB_QSIZE) {
		testbed.ttail = 0;
	}

	kprintf("testbed_assign: initializing request...\n");

	rqptr->type = TBR_TYPE_ASSIGN;
	kprintf("\tcopying dst %08x %08x\n", rqptr->dst, ethaddr);
	memcpy(rqptr->dst, topo[index].t_macaddr, 6);
	/*
	rqptr->nodeid = topo[index].t_nodeid;
	kprintf("\tcopying mcast\n");
	memcpy(rqptr->mcast, topo[index].t_neighbors, 6);
	for(i = 0; i < 46; i++) {
		kprintf("\tcopying link_info %d\n", i);
		rqptr->link_info[i].lqi_low = topo[index].link_info[i].lqi_low;
		rqptr->link_info[i].lqi_high = topo[index].link_info[i].lqi_high;
		rqptr->link_info[i].threshold = topo[index].link_info[i].threshold;
		rqptr->link_info[i].pathloss_ref = topo[index].link_info[i].pathloss_ref;
		rqptr->link_info[i].pathloss_exp = topo[index].link_info[i].pathloss_exp;
		rqptr->link_info[i].distance = topo[index].link_info[i].distance;
		rqptr->link_info[i].dist_ref = topo[index].link_info[i].dist_ref;
		rqptr->link_info[i].sigma = topo[index].link_info[i].sigma;	}
	*/
	rqptr->topoidx = index;
	rqptr->waiting = FALSE;

	restore(mask);

	kprintf("testbed_assign: sending request to testbed process\n");
	signal(testbed.tsem);

	return OK;
}

#if 0
/*-----------------------------------------------------------
 * Send an assign message as a response of JOIN message that is
 * received from a node.
 * ----------------------------------------------------------*/
status wsserver_assign ( struct netpacket *pkt )
{
    if (nodeid >= MAXNODES) {
        freebuf ((char *)pkt);
        return SYSERR;
    }

    struct etherPkt *assign_msg;
    assign_msg = create_etherPkt();
    int32 retval;
    int i;

    /* fill out Ethernet packet fields */
    /*DEBUG */ //for (i=0;i < 6;i++)
    //{
    //kprintf("%02x:", pkt->net_ethsrc[i]);
    //}
    memcpy ( assign_msg->src, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( assign_msg->dst, pkt->net_ethsrc, ETH_ADDR_LEN );
    assign_msg->type = htons ( ETH_TYPE_A );
    assign_msg->msg.amsgtyp = htonl ( A_ASSIGN ); /*Assign message */
    assign_msg->msg.anodeid = htonl ( nodeid );

    /*DEBUG *///kprintf("Assign type: %d:%d\n", htonl(A_ASSIGN), htonl(nodeid));
    for ( i = 0; i < 6; i++ ) {
        assign_msg->msg.amcastaddr[i] = topo[nodeid].t_neighbors[i];
        /*DEBUG *///kprintf("%02x:", assign_msg->amcastaddr[i]);
    }
    
    for ( i = 0; i < 46; i++)
    {
       assign_msg->msg.link_info[i].lqi_high = topo[nodeid].link_info[i].lqi_high;
       assign_msg->msg.link_info[i].lqi_low = topo[nodeid].link_info[i].lqi_low;
       assign_msg->msg.link_info[i].threshold = topo[nodeid].link_info[i].threshold;
       assign_msg->msg.link_info[i].pathloss_ref = topo[nodeid].link_info[i].pathloss_ref;
       assign_msg->msg.link_info[i].pathloss_exp = topo[nodeid].link_info[i].pathloss_exp;
       assign_msg->msg.link_info[i].distance = topo[nodeid].link_info[i].distance;
       assign_msg->msg.link_info[i].dist_ref = topo[nodeid].link_info[i].dist_ref;
       assign_msg->msg.link_info[i].sigma = topo[nodeid].link_info[i].sigma;
    }


    kprintf ( "\n*** Assigned Nodeid***: %d\n", nodeid );
    memset ( ack_info, 0, sizeof ( ack_info ) );
    memcpy ( ack_info, ( char * ) ( assign_msg ) + 14, 16 );
    retval = write ( ETHER0, ( char * ) assign_msg, sizeof (struct etherPkt) );
    //uint32 t1 = clktime;
    //kprintf("t1:%d\n", t1);
    freemem ( ( char * ) assign_msg, sizeof (struct etherPkt) );
    freebuf ( ( char * ) pkt );

    if ( retval > 0 ) {
        return OK;

    } else {
        return SYSERR;
    }
}
#endif
