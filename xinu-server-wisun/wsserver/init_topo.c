#include <xinu.h>
/*---------------------------------------------------------------
 * Initialize topology database with zeros
 * --------------------------------------------------------------*/

int32 bbb_stat[MAX_BBB];
void init_topo()
{
    int i, j, k;

    for ( i = 0; i < MAXNODES; i++ ) {
        topo[i].t_nodeid = i;
        topo[i].t_neighbors[0] = 1;

        for ( j = 1; j < 6; j++ )
            topo[i].t_neighbors[j] = 0;

        topo[i].t_status = 0;
        
	for ( k = 0; k < 46; k++)
	{
	   topo[i].link_info[k].lqi_high = 0;
	   topo[i].link_info[k].lqi_low = 0;
	   topo[i].link_info[k].threshold = 0;
	   topo[i].link_info[k].pathloss_ref = 0;
	   topo[i].link_info[k].pathloss_exp = 0;
	   topo[i].link_info[k].distance = 0;
	   topo[i].link_info[k].dist_ref = 0;
	   topo[i].link_info[k].sigma = 0;
	}
 
    }

    seqnum = ( int32 * ) getmem ( sizeof ( int32 ) );
    *seqnum = 0;

    for ( i = 0; i < 46; i++ )
        ping_ack_flag[i] = 0;

    for (i=0; i<MAX_BBB; i++)
	    bbb_stat[i] = 0;
}


