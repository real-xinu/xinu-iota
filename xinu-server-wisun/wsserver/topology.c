/* topology.c */

#include <xinu.h>

char	node2name[MAXNODES][NAMELEN];
char	map_list[MAXNODES][NAMELEN];

/*------------------------------------------------------------------------
 * topo_read  -  Read the topology from a remote file
 *------------------------------------------------------------------------
 */
int32	topo_read (
        char	*file,
        struct	topo_entry *topo
        )
{
    int32	fd;		/* File descriptor	*/
    int32	size;		/* File size		*/
    int32	rdcount;	/* Total bytes read	*/
    byte	namelen;	/* Node name length	*/
    int32	retval;		/* Return value		*/
    int32	i;		/* For loop index	*/
    int32	j = 0;		/* Current node id	*/

    /* Structre of entry in topology file */

    struct {
        byte	mcast[6];
        struct _linfo {
            byte lqi_low;
            byte lqi_high;
            byte threshold;
            byte pathloss_ref;
            byte pathloss_exp;
            byte dist_ref;
            byte sigma;
            uint32 distance;
        } linfo[46];
    } tent;

    byte	allzeros[] = {0, 0, 0, 0, 0, 0};

    /* Open the topology file */

    fd = open(RFILESYS, file, "ro");
    if(fd == SYSERR) {
        kprintf("topo_read: cannot open topology file: %s\n", file);
        return SYSERR;
    }

    /* Get the size of the file */

    size = control(RFILESYS, RFS_CTL_SIZE, 0, 0);
    if(size == SYSERR) {
        kprintf("topo_read: cannot get size of file: %s\n", file);
        return SYSERR;
    }
    
    //TODO: implement check here for size vs nodeids handed out (size % 552)

    kprintf("size of tent: %d, struct: %d\n", sizeof(tent) - 6, sizeof(tent.linfo));

    rdcount = 0;

    /* Read the topology entries from the file */
    
    while(rdcount < size) {
        retval = read(fd, (char *)&tent, 6);// reads multicast address
        if(retval == SYSERR) {
            kprintf("topo_read: error in reading file (multicast)\n");
            return SYSERR;
        }

        rdcount += retval;

        if(!memcmp(tent.mcast, allzeros, 6)) {
            break;
        }
        
        //multicast address is 6 bytes therefore +8 due to byte alignment
        retval = read(fd, (char *)&tent + 8, sizeof(tent.linfo));
        if(retval == SYSERR) {
            kprintf("topo_read: error in reading file (linkinfo)\n");
            return SYSERR;
        }

        rdcount += retval;

        kprintf("nodeid: %d, ", j);
        for(i = 0; i < 6; i++) {
            kprintf("%02x ", tent.mcast[i]);
        }
        kprintf("\n");

        for(i = 0; i < 6; i++) {
            if(tent.mcast[i] != 0) {
                break;
            }
        }
        if(i >= 6) {
            break;
        }


        memcpy(topo[j].t_neighbors, tent.mcast, 6);
        for(i = 0; i < 46; i++) {
            topo[j].link_info[i].lqi_low = tent.linfo[i].lqi_low;
            topo[j].link_info[i].lqi_high = tent.linfo[i].lqi_high;
            topo[j].link_info[i].threshold = tent.linfo[i].threshold;
			topo[j].link_info[i].pathloss_ref = tent.linfo[i].pathloss_ref;
			topo[j].link_info[i].pathloss_exp = tent.linfo[i].pathloss_exp;
			topo[j].link_info[i].distance = tent.linfo[i].distance;
			topo[j].link_info[i].dist_ref = tent.linfo[i].dist_ref;
			topo[j].link_info[i].sigma = tent.linfo[i].sigma;
        }
        j++;
    }

    nnodes = j;
    
    int32 check = 0;
    kprintf("newtop index 0\n");
    kprintf("LQI low 1: %d\n", topo[0].link_info[check].lqi_low);
    kprintf("LQI high 1: %d\n", topo[0].link_info[check].lqi_high);
    kprintf("Threshold 1: %d\n", topo[0].link_info[check].threshold);
    kprintf("pathloss_ref 1: %d\n", topo[0].link_info[check].pathloss_ref);
    kprintf("pathloss_exp 1: %d\n", topo[0].link_info[check].pathloss_exp);
    kprintf("dist_ref 1: %d\n", topo[0].link_info[check].dist_ref);
    kprintf("distance 1: %d\n", topo[0].link_info[check].distance);
    kprintf("sigma 1: %d\n", topo[0].link_info[check].sigma);
    check = 1;
    kprintf("newtop index 1\n");
    kprintf("LQI low 1: %d\n", topo[0].link_info[check].lqi_low);
    kprintf("LQI high 1: %d\n", topo[0].link_info[check].lqi_high);
    kprintf("Threshold 1: %d\n", topo[0].link_info[check].threshold);
    kprintf("pathloss_ref 1: %d\n", topo[0].link_info[check].pathloss_ref);
    kprintf("pathloss_exp 1: %d\n", topo[0].link_info[check].pathloss_exp);
    kprintf("dist_ref 1: %d\n", topo[0].link_info[check].dist_ref);
    kprintf("distance 1: %d\n", topo[0].link_info[check].distance);
    kprintf("sigma 1: %d\n", topo[0].link_info[check].sigma);
    
    kprintf("topo_read: reading names... rdcount %d size %d\n", rdcount, size);

    /* Read the node names from the file */

    j = 0;
    while(rdcount < size) {

        retval = read(fd, (char *)&namelen, 1);
        if(retval == SYSERR) {
            kprintf("topo_read: error in reading file\n");
            return SYSERR;
        }
        rdcount += retval;

        kprintf("topo_read: name length %d\n", (int32)namelen);
        retval = read(fd, map_list[j], (int32)namelen);
        if(retval == SYSERR) {
            kprintf("topo_read: error in reading file\n");
            return SYSERR;
        }
        rdcount += retval;

        kprintf("node name: %s\n", map_list[j]);

        j++;
    }

    close(fd);

    return OK;
}

/*------------------------------------------------------------------------
 * newtop - Handle incoming newtop command
 *------------------------------------------------------------------------
 */
struct	c_msg *newtop (
        struct	c_msg *cpkt	/* Incoming control packet */
        )
{
    struct	c_msg *reply;	/* Reply message	*/
    int32	retval;		/* Return value		*/
    int32	i;		/* For loop index	*/

    /* Allocate memory for reply */

    reply = (struct c_msg *)getmem(sizeof(reply));
    
    //TODO: need a corresponding check in cmsghandler.c to implement this
    /*
    if((int32)reply == SYSERR) {
        return SYSERR;
    }*/

    /* Save the old topology */

    memcpy(&old_topo, &topo, sizeof(topo));

    /* Read the new topology from file */

    retval = topo_read(cpkt->fname, &topo);
    if(retval == SYSERR) {
        memcpy(&topo, &old_topo, sizeof(topo));
        reply->cmsgtyp = htonl(C_ERR);
        return reply;
    }

    /* Compare the new topology with old topology */

    for(i = 0; i < MAXNODES; i++) {

        if(topo[i].t_status == 0) {
            continue;
        }

        memcpy(topo[i].t_macaddr, old_topo[i].t_macaddr, 6);

        if( (memcmp(topo[i].t_neighbors, old_topo[i].t_neighbors, 6)) ||
                (memcmp(topo[i].link_info, old_topo[i].link_info,
                        sizeof(topo[i].link_info))) ) {

            /* Send an Assign message to this node */

            kprintf("newtop: calling assign on %d\n", i);
            retval = testbed_assign(i, NULL);
            if(retval == SYSERR) {
                reply->cmsgtyp = htonl(C_ERR);
                return reply;
            }
        }
    }
    //TODO: add code to check if the assignment is ok before returning OK
    reply->cmsgtyp = htonl(C_OK);

    return reply;
}

/*------------------------------------------------------------------------
 * toporeply  -  Send a response to topology request
 *------------------------------------------------------------------------
 */
struct	c_msg *toporeply (void) {

    struct	c_msg *reply;	/* Reply message	*/
    int32	i;

    reply = (struct c_msg *) getmem(sizeof(struct c_msg));

    reply->cmsgtyp = htonl(C_TOP_REPLY);
    reply->topnum = htonl(nnodes);

    for(i = 0; i < nnodes; i++) {

        reply->topdata[i].t_nodeid = htonl(topo[i].t_nodeid);
        reply->topdata[i].t_status = htonl(topo[i].t_status);
        reply->topdata[i].t_bbbid = htonl(topo[i].t_bbbid);
        memcpy(reply->topdata[i].t_neighbors, topo[i].t_neighbors, 6);
    }

    return reply;
}
