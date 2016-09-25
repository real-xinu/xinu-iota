#include <xinu.h>

/*---------------------------------------------------------------------------------
 * controle message handler is used to call appropiate function based on message
 * which is received from the management server.
 * ----------------------------------------------------------------------------*/
struct c_msg * cmsg_handler ( struct c_msg *ctlpkt )
{
    int xonoffid;
    struct c_msg *cmsg_reply;
    int32 mgm_msgtyp = ntohl ( ctlpkt->cmsgtyp );
    cmsg_reply = ( struct c_msg * ) getmem ( sizeof ( struct c_msg ) );
    memset ( cmsg_reply, 0, sizeof ( struct c_msg ) );

    /*------------------------------------------------------
     * The type of control message should be checked here
     * and then an appropiate response message should be created
     * returned to the caller.
     * ----------------------------------------------------*/
    switch ( mgm_msgtyp ) {
        case C_RESTART:
            kprintf ( "Message type is %d\n", C_RESTART );
            //cmsg_reply->cmsgtyp = htonl(C_OK);
            break;

        case C_RESTART_NODES:
            kprintf ( "Message type is %d\n", C_RESTART_NODES );

            if ( online )
                cmsg_reply->cmsgtyp = htonl ( C_OK );

            break;

        case C_PING_REQ:
            kprintf ( "Message type is %d\n", C_PING_REQ );

            if ( online )
                cmsg_reply = nping_reply ( ctlpkt );

            break;

        case C_PING_ALL:
            kprintf ( "Message type is %d\n", C_PING_ALL );

            if ( online )
                cmsg_reply = nping_all_reply ( ctlpkt );

            break;

        case C_XOFF:
            kprintf ( "Message type is %d\n", C_XOFF );

            if ( online ) {
                xonoffid = ntohl (ctlpkt->xonoffid);

                if (wsserver_xonoff (XOFF, xonoffid) == OK)
                    cmsg_reply->cmsgtyp = htonl ( C_OK );

                else
                    cmsg_reply->cmsgtyp = htonl (C_ERR);
            }

            break;

        case C_XON:
            kprintf ( "Message type is %d\n", C_XON );

            if ( online ) {
                xonoffid = ntohl (ctlpkt->xonoffid);

                if (wsserver_xonoff (XON, xonoffid) == OK)
                    cmsg_reply->cmsgtyp = htonl ( C_OK );

                else
                    cmsg_reply->cmsgtyp = htonl (C_ERR);
            }

            break;

        case C_OFFLINE:
            online = 0;
            kprintf ( "Message type is %d\n", C_OFFLINE );
            cmsg_reply->cmsgtyp = htonl ( C_OK );
            break;

        case C_ONLINE:
            online = 1;
            kprintf ( "Message type is %d\n", C_ONLINE );
            cmsg_reply->cmsgtyp = htonl ( C_OK );
            break;

        case C_TOP_REQ:
            kprintf ( "Message type is %d\n", C_TOP_REQ );
            cmsg_reply = toporeply();
            break;

        case C_NEW_TOP:
            kprintf ( "Message type is %d\n", C_NEW_TOP );
            cmsg_reply = newtop ( ctlpkt );
            break;

        case C_TS_REQ:
            kprintf ( "Message type is %d\n", C_TS_REQ );
            cmsg_reply->cmsgtyp = htonl ( C_TS_RESP );
	    cmsg_reply->uptime  = htonl(clktime);
            break;

        case C_MAP:
            kprintf (" Message type is %d\n", C_MAP);
            cmsg_reply->cmsgtyp = htonl (C_MAP_REPLY);
            cmsg_reply->nnodes = htonl (nnodes);
            memcpy (cmsg_reply->map, map_list, sizeof (map_list));
            break;

        case C_SHUTDOWN:
            kprintf ("Message type is %d\n", C_SHUTDOWN);
	    cmsg_reply->cmsgtyp = htonl (C_SHUTDOWN);
            break;

        case C_PINGALL:
            kprintf ("Message type is %d\n", C_PINGALL);
            cmsg_reply = pingall_reply ( ctlpkt );
	    break;
        case C_CLEANUP:
            kprintf ("Message type is %d\n", C_CLEANUP);
	    if (cleanup() == OK)
		    cmsg_reply->cmsgtyp = htonl ( C_OK );
	    else
                    cmsg_reply->cmsgtyp = htonl ( C_ERR );
	    break;

        default:
            kprintf ( "ERROR\n" );
            cmsg_reply->cmsgtyp = htonl ( C_ERR );
    }

    return cmsg_reply;
}


