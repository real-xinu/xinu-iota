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

        kprintf (" ");
    }

        kprintf("\nLink Info (lqi_low, lqi_high, probloss) : ");
	for ( i = 0; i < 46; i++)
	{
	   kprintf(" %d, %d, %d  ",info.link_info[i].lqi_low ,info.link_info[i].lqi_high ,info.link_info[i].probloss);

	}

	kprintf("\n");
 
}



