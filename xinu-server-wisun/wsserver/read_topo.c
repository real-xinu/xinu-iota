#include <xinu.h>

/*------------------------------------------------------------------------
*Use the remote file system to open and read a topology database file
--------------------------------------------------------------------------*/
status read_topo ( char *filename )
{
    char *buff;
    uint32 size;
    int status = read_topology ( filename, &buff, &size );

    if ( status == SYSERR ) {
        kprintf ( "WARNING: Could not read topology file\n" );
        return SYSERR;
    }

    nnodes = topo_update ( buff, size, topo );
    freemem ( buff, size );
    return OK;
}
