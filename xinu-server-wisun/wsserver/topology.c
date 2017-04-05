/* topology.c */

#include <xinu.h>

char	node2name[MAXNODES][NAMELEN];

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
		  byte	lqi_low;
		  byte	lqi_high;
		  byte	loss;
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

	rdcount = 0;

	/* Read the topology entries from the file */

	while(rdcount < size) {

		retval = read(fd, (char *)&tent, 6);
		if(retval == SYSERR) {
			kprintf("topo_read: error in reading file\n");
			return SYSERR;
		}

		rdcount += retval;

		if(!memcmp(tent.mcast, allzeros, 6)) {
			break;
		}

		retval = read(fd, (char *)&tent + 6, sizeof(tent) - 6);
		if(retval == SYSERR) {
			kprintf("topo_read: error in reading file\n");
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
			topo[j].link_info[i].probloss = tent.linfo[i].loss;
		}
		j++;
	}

	nnodes = j;

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
	if((int32)reply == SYSERR) {
		return SYSERR;
	}

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

			retval = testbed_assign(topo[i].t_macaddr);
			if(retval == SYSERR) {
				reply->cmsgtyp = htonl(C_ERR);
				return reply;
			}
		}
	}

	reply->cmsgtyp = htonl(C_OK);

	return reply;
}
