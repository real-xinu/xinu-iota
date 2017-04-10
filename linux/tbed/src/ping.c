#include "../include/headers.h"
#include "../include/prototypes.h"
#include "../include/macros.h"
#include "../include/global.h"




/*-----------------------------------------------------------------------------------------------
 * ping reply control message which is received from testbed server is handled in this function
 * it prints the staus of a node or a set of nodes  that have been  pinged.
 -----------------------------------------------------------------------------------------------*/

void ping_reply_handler (struct c_msg *buf)
{
    int i, counter, status;
    counter = ntohl (buf->pingnum);

    fprintf(fp, "ping_reply_handler: pingnum: %d\n", counter);
    for (i = 0; i < counter; i++) {
        status = ntohl (buf->pingdata[i].pstatus);

        if (status == ALIVE) {
            fprintf (fp, "<====Reply from testbed server: Node %d is alive\n", ntohl (buf->pingdata[i].pnodeid));

        } else if ((status  == NOTACTIV) && (counter == 1)) {
            fprintf (fp, "<====Reply from testbed server: Node %d is not in the active network topology\n", ntohl (buf->pingdata[i].pnodeid));

        } else if (status == NOTRESP) {
            fprintf (fp, "<====Reply from testbed server: Node %d is not responding \n", ntohl (buf->pingdata[i].pnodeid));
        }
    }
}

/* pingall response handler */
void pingall_handler (struct c_msg *buf)
{
    int i, counter;
    counter = ntohl (buf->nbbb);

    fprintf(fp, "number of beagles: %d\n", counter);

    for (i = 0; i < MAX_BBB; i++) {
	    if (buf->bbb_stat[i] == 1)
                 fprintf(fp , "beagle%d is on.\n", i + 101);
    }
}


