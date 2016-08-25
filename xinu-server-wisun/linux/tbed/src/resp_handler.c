#include "../include/headers.h"
#include "../include/prototypes.h"
#include "../include/macros.h"
#include "../include/global.h"




/*-----------------------------------------------------------*
 *  Control message response handler is used to handle
 *  responses from the testbed server
 *  ---------------------------------------------------------*/

void response_handler (struct c_msg *buf)
{
    int32 cmsgtyp = ntohl (buf->cmsgtyp);

    /*-------------------------------------------
     * Control message type should be checked and
     * an appropiate function should be called to
     * handle it
     * ------------------------------------------*/
    switch (cmsgtyp) {
        case C_OK:
            fprintf (fp, "<====Reply from testbed server: OK\n");
            break;

        case C_ERR:
            fprintf (fp, "<====Reply from testbed server:ERROR\n");
            break;

        case C_TOP_REPLY:
            topodump (buf);
            break;

        case C_PING_REPLY:
            ping_reply_handler (buf);
            break;

        case C_PING_ALL:
            ping_reply_handler (buf);
            break;
	case C_PINGALL_REPLY:
            pingall_handler (buf);
	    break;

        case C_MAP_REPLY:
            getmap (buf);
            break;
    }
}



