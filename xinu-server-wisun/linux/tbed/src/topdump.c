#include "../include/headers.h"
#include "../include/global.h"
#include "../include/prototypes.h"
#include "../include/macros.h"
/*-----------------------------------------------------
* Print the network topology based on control message
which have been recevied from the testbed server.
*-----------------------------------------------------*/
void topodump (struct c_msg *buf)
{
    fflush (stdout);

    if (ntohl (buf->topnum) == 0) {
        fprintf (fp, "Please enter newtop command\n");
    }

    /* -------------------------------------------------------------
     * Print the number of nodes in the active network topolog
     * -----------------------------------------------------------*/
    if (ntohl (buf->topnum) > 0) {
        fprintf (fp, "Number of Nodes: %d\n", ntohl (buf->topnum));
    }

    int entries = ntohl (buf->topnum);
    byte mcaddr[6];

    for (int i = 0; i < entries; i++) {
        fprintf (fp, "%d ,", ntohl (buf->topdata[i].t_nodeid));
        fprintf (fp, "%d ,", ntohl (buf->topdata[i].t_status));

        for (int j = 0; j < 6; j++) {
            for (int k = 7; k >= 0; k--) {
                fprintf (fp, "%d", (buf->topdata[i].t_neighbors[j] >> k) & 0x01);
            }

            fprintf (fp, " ");
            mcaddr[j] = buf->topdata[i].t_neighbors[j];
        }

        fprintf (fp, "\nThe neighbors of node %d are: ", ntohl (buf->topdata[i].t_nodeid));

        for (int j = 0; j < 46; j++) {
            if (srbit (mcaddr, j, BIT_TEST) == 1) {
                fprintf (fp, "%d ", j);
            }
        }

        fprintf (fp, "\n");
    }
}



