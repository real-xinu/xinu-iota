/* wstopology.c - Functions for reading and working with topology files */

#include <xinu.h>

/***********************************************************************
 * read_topology: Read a topology file from a remote file server
 * EXAMPLE USAGE:
 *      char *buff;
 *      int nodes;
 *      int status = read_topology("filename", &buff, &nodes);
 ***********************************************************************/
status read_topology(char *name, char **buff, uint32 *size)
{
    /* Open topology file from server */

    int32 fd = open(RFILESYS, name, "ro");
    if (fd == SYSERR) {
        printf("WARNING: Could not open topology file for reading\n");
        return SYSERR;
    }

    /* Get size of topology file so we know how much to read */
    *size = control(RFILESYS, RFS_CTL_SIZE, 0, 0);

    if (*size == SYSERR) {
        printf("WARNING: Could not get topology file size\n");
        close(fd);
        return SYSERR;
    }

    /* Allocate buffer so that caller knows where it is */
    *buff = getmem(*size);
    if ((int32) *buff == SYSERR) {
        printf("WARNING: Could not allocate memory for topology\n");
        close(fd);
        return SYSERR;
    }

    /* Read the topology file into the buffer */
    int32 status = read(fd, *buff, *size);
    if (status == SYSERR) {
        printf("WARNING: Could not read topology file contents\n");
        close(fd);
        freemem(*buff,*size);
        return SYSERR;
    }

    close(fd);
    return OK;
}

/***********************************************************************
 * print_raw_topology: Print MAC addresses in a binary topology
 *  (this is mostly useful for testing)
 ***********************************************************************/
int32 topo_update(char *buff, uint32 size, struct t_entry *topo)
{
    int i, j;

    int nnodes = 46;

    int32 num_of_entries;

    int counter = 0;
    int flag = 0;

    int len_flag = 0;
    int name_size;
    i = 0;
    while (counter <= size) {
        if (flag != -1) {
            for(j = 0; j < ETH_ADDR_LEN; j++) {
                /*
                       for (k=7; k>=0; k--)
                       {
                       	kprintf("%d ", (buff[i * ETH_ADDR_LEN + j]>>k)&0x01);
                       }
                       kprintf(" ");
                */
                if ((int)buff[i * ETH_ADDR_LEN +j] == 0)
                    flag++;
            }
            //kprintf("\n");
            if (flag == 6)
                flag = -1;
            else {
                flag = 0;
                topo[i].t_nodeid = i;
                topo[i].t_status = 0;
                for (j=0; j<6; j++) {
                    topo[i].t_neighbors[j] = (unsigned char)buff[counter + j];
                }
                i++;
            }
            counter = counter + 6;
        } else {
            if (len_flag == 0) {
                name_size = buff[counter];
                counter++;
                len_flag = 1;
            } else {
                //int name[name_size];

                for (j=0; j<name_size; j++) {
                    //name[j] = buff[counter + j];
                    //kprintf("%s",&name[j]);
                }
                //kprintf("\n");
                counter = counter + name_size;
                len_flag = 0;
            }
        }
    }
    num_of_entries = i;
    for (i= num_of_entries -1; i<nnodes; i++) {
        topo[i].t_nodeid = i;
        topo[i].t_status = 0;
    }
    return num_of_entries;
}
