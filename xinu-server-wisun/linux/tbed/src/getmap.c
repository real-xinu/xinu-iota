#include "../include/headers.h"
#include "../include/prototypes.h"
#include "../include/macros.h"
#include "../include/global.h"


char map_list[46][20];
/* map reply control message which is recevied from the the testbed server
 * is handled in this function to print the names for each of the nodes */

void getmap (struct c_msg *buf)
{
    nnodes = ntohl (buf->nnodes);
    memcpy (map_list, buf->map, sizeof (buf->map));
    int i;
    printf ("nnodes:%d\n", nnodes);

    for (i = 0; i < nnodes; i++) {
        fprintf (fp, "%s ", map_list[i]);
    }

    //fprintf(fp, "%s", "\n");
}
