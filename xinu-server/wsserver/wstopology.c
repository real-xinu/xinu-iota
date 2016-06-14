/* wstopology.c - Functions for reading and working with topology files */

#include <xinu.h>

/***********************************************************************
 * read_topology: Read a topology file from a remote file server
 * EXAMPLE USAGE:
 *      char *buff;
 *      int nodes;
 *      int status = read_topology("filename", &buff, &nodes);
 ***********************************************************************/
status read_topology(char *name, char **buff, int *nodes)
{
    /* Open topology file from server */
    int32 fd = open(RFILESYS, name, "ro");
    if (fd == SYSERR) {
        printf("WARNING: Could not open topology file for reading\n");
        return SYSERR;
    }

    /* Get size of topology file so we know how much to read */
    int32 size = control(RFILESYS, RFS_CTL_SIZE, 0, 0);
    if (size == SYSERR) {
        printf("WARNING: Could not get topology file size\n");
        close(fd);
        return SYSERR;
    }

    /* Allocate buffer so that caller knows where it is */
    *buff = getmem(size);
    if ((int32) *buff == SYSERR) {
      printf("WARNING: Could not allocate memory for topology\n");
      close(fd);
      return SYSERR;
    }

    /* Read the topology file into the buffer */
    int32 status = read(fd, *buff, size);
    if (status == SYSERR) {
        printf("WARNING: Could not read topology file contents\n");
        close(fd);
        freemem(*buff, size);
        return SYSERR;
    }

    /* Set number of nodes for caller */
    *nodes = size / ETH_ADDR_LEN;

    close(fd);
    return OK;
}

/***********************************************************************
 * print_raw_topology: Print MAC addresses in a binary topology
 *  (this is mostly useful for testing)
 ***********************************************************************/
void print_raw_topology(char *buff, int nodes)
{
    int i, j;

    for (i = 0; i < nodes; i++) {
        printf("%d: ", i);
        for(j = 0; j < ETH_ADDR_LEN; j++)
            printf("%02x ", buff[i * ETH_ADDR_LEN + j]);
        printf("\n");
    }

}
