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
}



