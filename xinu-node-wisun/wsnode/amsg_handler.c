#include <xinu.h>
#include <stdlib.h>
/*------------------------------------------------
 * Message handler is used to call
 * appropiate function based on message type.
 * -----------------------------------------------*/
void amsg_handler ( struct netpacket_e *node_msg )
{
    float delay;
    int32 amsgtyp = ntohl ( node_msg->msg.amsgtyp );
    int i;

    /*--------------------------------------------------------------------------
     * The message type for type A frames  should be check here an appropiate
     * function should be called to handle the request.
     * -----------------------------------------------------------------------*/
    switch ( amsgtyp ) {
        case A_ASSIGN:
            info.nodeid = ntohl ( node_msg->msg.anodeid );

            if ( wsnode_sendack (node_msg) == OK ) {
                for ( i = 0; i < 6; i++ ) {
                    info.mcastaddr[i] = node_msg->msg.amcastaddr[i];
                    kprintf ("%02x:", node_msg->msg.amcastaddr[i]);
                }

                kprintf ("\n");
                print_info();
                kprintf ( "\n====>ACK message is sent\n" );
            }

            break;

        case A_RESTART:
            kprintf ( "<==== RESTART message is received\n" );
            freebuf ((char *) node_msg);
            break;

        case A_XOFF:
            xon = 0;
            kprintf ( "<====XOF message is received\n" );
            freebuf ((char *) node_msg);
            break;

        case A_XON:
            xon = 1;
            kprintf ( "<==== XON message is received\n" );
            freebuf ((char *) node_msg);
            break;

        case A_PING:
            kprintf ( "<==== PING message is received\n" );

            if ( wsnode_sendack (node_msg) == OK ) {
                kprintf ( "====> ACK message is sent\n" );
            }

            break;

        case A_PINGALL:
            delay = (rand() % 20) * 0.001;
            sleep ( delay );
            kprintf ( "<==== PINGALL ALL message is received\n" );

            if ( wsnode_sendack (node_msg ) == OK ) {
                kprintf ( "====> ACK message is sent\n" );
            }

            break;

        case A_PING_ALL:
            delay = ( info.nodeid * 0.001 );
            sleep ( delay );
            kprintf ( "<==== PINGALL message is received\n" );

            if ( wsnode_sendack (node_msg ) == OK ) {
                kprintf ( "====> ACK message is sent\n" );
            }

            break;

        default:
            freebuf ((char *) node_msg);
            break;
    }
}


