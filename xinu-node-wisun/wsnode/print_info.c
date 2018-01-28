#include <xinu.h>

/*-------------------------------------------------------------------
 A utility function to print multicast address and node ID
-----------------------------------------------------------------*/
void print_info()
{
    int i, j;
    /*DEBUG */    kprintf ( "node id:%d\n", info.nodeid );

    /*DEBUG */    for ( j = 0; j < 6; j++ ) {
        for ( i = 7; i >= 0; i-- ) {
            kprintf ("%d ", (info.mcastaddr[j] >> i) & 0x01);
        }

        kprintf (" \n");
    }
    for(i = 0; i < 46; i++) {
    	kprintf("%d:\nLQI: Low: %d, High:%d\nThreshold: %d\nPathloss: Ref: %d, Exp:%d\nDist_ref: %d, Sigma: %d\nDistance: %d\n",
                i, info.link_info[i].lqi_low, info.link_info[i].lqi_high, 
                info.link_info[i].threshold, info.link_info[i].pathloss_ref, 
                info.link_info[i].pathloss_exp, info.link_info[i].dist_ref,
                info.link_info[i].sigma, info.link_info[i].distance);
    }
}
