#include <xinu.h>

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

        kprintf ( " --Link Info: ");
        for ( j = 0; j < 46; j++ )
            kprintf ( "%d,%d,%d ", old_topo[i].link_info[j].lqi_low, old_topo[i].link_info[j].lqi_high, old_topo[i].link_info[j].threshold);

        kprintf ( "  :  " );

        for ( j = 0; j < 46; j++ )
            kprintf ( "%d,%d,%d ", topo[i].link_info[j].lqi_low, topo[i].link_info[j].lqi_high, topo[i].link_info[j].threshold);

/*


        kprintf ( "\n  -- macaddr:  " );

        for ( j = 0; j < 6; j++ )
            kprintf ( "%02x ", old_topo[i].t_macaddr[j] );

        kprintf ( "  :  " );

        for ( j = 0; j < 6; j++ )
            kprintf ( "%02x ", topo[i].t_macaddr[j] );
*/
        kprintf ( "\n" );
    }
}


