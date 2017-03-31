/* wstopology.c - Functions for reading and working with topology files */

#include <xinu.h>
#include <string.h>
char map_list[MAXNODES][NAMELEN];

/***********************************************************************
 * read_topology: Read a topology file from a remote file server
 * EXAMPLE USAGE:
 *      char *buff;
 *      int nodes;
 *      int status = read_topology("filename", &buff, &nodes);
 ***********************************************************************/
status read_topology ( char *name, char **buff, uint32 *size )
{
   
    int32 status;
    char * temp;
    uint32 res_size;

    /* Open topology file from server */
    int32 fd = open ( RFILESYS, name, "ro" );

    if ( fd == SYSERR ) {
        printf ( "WARNING: Could not open topology file for reading\n" );
        return SYSERR;
    }

    /* Get size of topology file so we know how much to read */
    *size = control ( RFILESYS, RFS_CTL_SIZE, 0, 0 );

    if ( *size == SYSERR ) {
        printf ( "WARNING: Could not get topology file size\n" );
        close ( fd );
        return SYSERR;
    }

    /* Allocate buffer so that caller knows where it is */
    *buff = getmem ( *size );

    if ( ( int32 ) *buff == SYSERR ) {
        printf ( "WARNING: Could not allocate memory for topology\n" );
        close ( fd );
        return SYSERR;
    }

    /* Read the topology file into the buffer */
    if (*size < RF_DATALEN)
        status = read ( fd, *buff, *size );
    else
    {
        res_size = *size;
	temp = *buff;
	while (res_size >= RF_DATALEN)
        {
	  status = read ( fd, temp, RF_DATALEN - 1 );
	  res_size = res_size - (RF_DATALEN - 1);
	  temp += (RF_DATALEN - 1);
	}
	status = read ( fd, temp, res_size );
    }

    if ( status == SYSERR ) {
        printf ( "WARNING: Could not read topology file contents\n" );
        close ( fd );
        freemem ( *buff, *size );
        return SYSERR;
    }

    close ( fd );
    return OK;
}

/***********************************************************************
 * print_raw_topology: Print MAC addresses in a binary topology
 *  (this is mostly useful for testing)
 ***********************************************************************/
int32 topo_update ( char *buff, uint32 size, struct topo_entry *topo )
{
    int i, j, k, m;
    int nnodes = 46;
    int32 num_of_entries;
    int counter = 0;
    int flag = 0;
    int len_flag = 0;
    int name_size;
    i = 0;
    k = 0;

    while ( counter <= size ) {
        if ( flag != -1 ) {
            for ( j = 0; j < ETH_ADDR_LEN; j++ ) {
                /*DEBUG*/ /*  
                          for (k=7; k>=0; k--)
                          {
                          	kprintf("%d ", (buff[counter+ j]>>k)&0x01);
                          }
			  kprintf(" ");
                   */
                if ( ( int ) buff[counter + j] == 0 )
                    flag++;
            }
            //kprintf("\n");
            if ( flag == 6 )
	    {
                flag = -1;
                counter = counter + 6;
            }
            else {

                flag = 0;
                topo[i].t_nodeid = i;
                topo[i].t_status = 0;

                for ( j = 0; j < 6; j++ ) {
                    topo[i].t_neighbors[j] = ( unsigned char ) buff[counter + j];
                }
                counter = counter + 6;
            	for (m = 0;m < 46; m++)
	    	{
	        	topo[i].link_info[m].lqi_low = ( unsigned char ) buff[counter];
			counter++;
                	topo[i].link_info[m].lqi_high = ( unsigned char ) buff[counter];
			counter++;
                	topo[i].link_info[m].probloss = ( unsigned char ) buff[counter];
			counter++;

			/* DEBUG */// kprintf("link_info: %d, %d, %d  ", topo[i].link_info[m].lqi_low, topo[i].link_info[m].lqi_high, topo[i].link_info[m].probloss);
	    	}
	    	//kprintf("\n\n");


                i++;
            }



        } else {
            if ( len_flag == 0 ) {
                name_size = buff[counter];
                counter++;
                len_flag = 1;

            } else {
                //int name[name_size];
                for ( j = 0; j < name_size; j++ ) {
                    /*DEBUG */ //name[j] = buff[counter + j];
                    /*DEBUG */ //kprintf("%s",&name[j]);
                    map_list[k][j] =  buff[counter + j];
                }

                /*DEBUG */  //kprintf("%s", &map_list[k]);
                k++;
                //kprintf("\n");
                counter = counter + name_size;
                len_flag = 0;
            }
        }
    }

    num_of_entries = i;

    for ( i = num_of_entries - 1; i < nnodes; i++ ) {
        topo[i].t_nodeid = i;
        topo[i].t_status = 0;
    }

    return num_of_entries;
}
