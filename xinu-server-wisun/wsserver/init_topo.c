#include <xinu.h>
/*---------------------------------------------------------------
 * Initialize topology database with zeros
 * --------------------------------------------------------------*/
void init_topo()
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


