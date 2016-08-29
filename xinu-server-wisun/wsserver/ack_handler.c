#include <xinu.h>

/*----------------------------------------------------------------
 *  ACK handler
 *  -------------------------------------------------------------*/
void ack_handler ( struct netpacket *pkt )
{
    struct etherPkt *node_msg;
    node_msg = ( struct etherPkt * ) pkt;
    int i,j, ack;
    ack = 0;
    int mac = 0;

    for ( i = 0; i < 16; i++ ) {
        if ( node_msg->msg.aacking[i] == ack_info[i] ) {
            ack++;
        }

        /*DEBUG*/ //kprintf("aacking:%d:%d\n", node_msg->aacking[i],ack_info[i]);
    }



    if ( ack == 16 ) {
        if ( ack_info[3] == A_ASSIGN ) {
            topo_update_mac ( pkt );
            kprintf ( "====>Assign ACK message is received\n" );

        } else if ( ack_info[3] == A_PING ) {
            i = ntohl ( node_msg->msg.anodeid );
            ping_ack_flag[i] = 1;
            freebuf ( ( char * ) pkt );
            kprintf ("====>Ping ACK message is received\n");

        } else if ( ack_info[3] == A_PING_ALL ) {
            i = ntohl ( node_msg->msg.anodeid );
            ping_ack_flag[i] = 1;
            freebuf ( ( char * ) pkt );
        } else if (ack_info[3] == A_PINGALL) {
	   /* //DEBUG
	    for (j=0; j<6; j++)
		    kprintf("%02x:", node_msg->src[j]);
	    kprintf("\n");
	    */
	    for (i = 0; i< MAX_BBB; i++)
	    {
		    mac = 0;
		    for (j = 0; j<6; j++)
		    {
			    if (node_msg->src[j] == bbb_macs[i][j])
				    mac++;
		    }
		    if (mac == 6)
		    {
			    bbb_stat[i] = 1;
			    break;
		    }			    
	    }
	    freebuf((char *)pkt);
	}

    } else
        freebuf ( ( char * ) pkt );
}


