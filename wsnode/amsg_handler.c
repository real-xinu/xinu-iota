#include <xinu.h>
#include <stdlib.h>
/*------------------------------------------------
 * Message handler is used to call
 * appropiate function based on message type.
 * -----------------------------------------------*/
uint32	glbtime;
extern sid32 sem1;
void amsg_handler ( struct netpacket_e *node_msg )
{
    float delay;
    int32 amsgtyp = ntohl ( node_msg->msg.amsgtyp );
    int i;
    uint32 rseed;

    /*--------------------------------------------------------------------------
     * The message type for type A frames  should be check here an appropiate
     * function should be called to handle the request.
     * -----------------------------------------------------------------------*/
    switch ( amsgtyp ) {
        case A_ASSIGN:
            info.nodeid = ntohl ( node_msg->msg.anodeid );

            if ( wsnode_sendack (node_msg, A_ASSIGN) == OK ) {
                for ( i = 0; i < 6; i++ ) {
                    info.mcastaddr[i] = node_msg->msg.amcastaddr[i];
                    kprintf ("%02x:", node_msg->msg.amcastaddr[i]);
                }

		for(i = 0; i < 46; i++) {
		    info.link_info[i].lqi_low = node_msg->msg.link_info[i].lqi_low;
		    info.link_info[i].lqi_high = node_msg->msg.link_info[i].lqi_high;
		    info.link_info[i].probloss = node_msg->msg.link_info[i].probloss;
		}
                kprintf ("\n");
                print_info();
                kprintf ( "\n====>ACK message is sent\n" );
            }

            break;

        case A_RESTART:
            kprintf ( "<==== RESTART message is received\n" );
            //freebuf ((char *) node_msg);
            break;

        case A_XOFF:
            xon = 0;
            kprintf ( "<====XOF message is received\n" );
            //freebuf ((char *) node_msg);
            break;

        case A_XON:
            xon = 1;
            kprintf ( "<==== XON message is received\n" );
            //freebuf ((char *) node_msg);
            break;

        case A_PING:
            kprintf ( "<==== PING message is received\n" );

            if ( wsnode_sendack (node_msg, A_PING) == OK ) {
                kprintf ( "====> ACK message is sent\n" );
            }

            break;

        case A_PINGALL:
            delay = (rand() % 20) * 0.001;
            sleep ( delay );
            kprintf ( "<==== PINGALL ALL message is received\n" );

            if ( wsnode_sendack (node_msg, A_PINGALL ) == OK ) {
                kprintf ( "====> ACK message is sent\n" );
            }

            break;

        case A_PING_ALL:
            delay = ( info.nodeid * 0.001 );
            sleep ( delay );
            kprintf ( "<==== PINGALL message is received\n" );

            if ( wsnode_sendack (node_msg, A_PING_ALL ) == OK ) {
                kprintf ( "====> ACK message is sent\n" );
            }

            break;
        case A_CLEANUP:
            panic("The node is down\n");
	    break;
	case A_SETTIME:
	    glbtime = ntohl(node_msg->msg.ctime);
	    kprintf("<==== SETTIME: %d, %x\n", glbtime, glbtime);
	    rseed = *((uint32 *)&iftab[0].if_hwucast[2]);
	    kprintf("rseed: %x\n", rseed);
	    rseed += glbtime;
	    kprintf("rseed: %d\n", rseed);
            srand(rseed);

        default:
            //freebuf ((char *) node_msg);
            break;
    }
}


