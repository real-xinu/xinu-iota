#include "../include/headers.h"
#include "../include/prototypes.h"
#include "../include/macros.h"
#include "../include/global.h"

char *imgpath = "../images/";

/*------------------------------------------------------------------------------
 *
 *download_img: dowload an image (node or testbed server image) to a specefic backend
 *-------------------------------------------------------------------------------*/
int download_img (char * filename, char * class, char * connection, char * host)
{
    pid_t pid_dwnld;
    pid_dwnld = fork();

    if (pid_dwnld == 0) {
        /* Child Process */
        char conn[128];
        int fd;
        strcat (imgpath, filename);
        printf ("filename:%s\n", filename);

        if ( ( fd = open (filename, O_RDONLY ) ) < 0 ) {
            perror ( "open()" );
            exit ( 1 );
        }

        if ( dup2 ( fd, 0 ) < 0 ) {
            perror ( "dup2()" );
            exit ( 1 );
        }

        close ( fd );
        sprintf (conn, "%s%s", connection, "-dl");
        execlp (EXEC, EXEC, "-t", "-f", "-c", DOWNLOAD, "-s", host, conn, NULL);
        fprintf (stderr, "execlp() failes\n");
        return -1;

    } else if (pid_dwnld < 0) {
        fprintf (stdout, "\nfork() Error\n");
        return -1;
    }

    return 1;
}
/*----------------------------------------------------------
 * Powercycle_bgnd: powercycle a backend
 * --------------------------------------------------------*/

int powercycle_bgnd (char * class, char * connection, char * host)
{
    pid_t pid_dwnld;
    pid_dwnld = fork();

    /* Child process to execute cs-console command */
    if (pid_dwnld == 0) {
        /* Child Process */
        char conn[128];
        sprintf (conn, "%s%s", connection, "-pc");
        execlp (EXEC, EXEC, "-t", "-f", "-c", POWERCYCLE, "-s", host, conn, NULL);
        fprintf (stderr, "execlp() failes\n");
        return -1;

    } else if (pid_dwnld < 0) {
        fprintf (stdout, "\nfork() Error\n");
        return -1;
    }

    return 1;
}

/*-------------------------------------------------------------
 * Connect to backened to download an image or powercycle it
 *-----------------------------------------------------------*/
int connect_bgnd (char * class, char * connection, char * host)
{
    /* Make connection */
    int ret = makeConnection (connection, class, host);
    return ret;
}


