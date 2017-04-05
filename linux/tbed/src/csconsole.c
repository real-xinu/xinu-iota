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
    char *file_name_path = malloc (1 + strlen (imgpath) + strlen (filename));
    pid_t pid_dwnld;
    pid_dwnld = fork();
    /*DEBUG */ //printf ("id:%d\n", pid_dwnld);

    //fflush(stdout);
    if (pid_dwnld == 0) {
        /* Child Process */
        /*DEBUG */ //printf ("filename:%s\n", filename);
        char conn[128];
        int fd;
        strcpy (file_name_path, imgpath);
        strcat (file_name_path, filename);

        //fprintf (stdout,"filename:%s\n", filename);
        //printf("hello\n");
        //fflush(stdout);
        if ( ( fd = open (file_name_path, O_RDONLY ) ) < 0 ) {
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
        fflush (stdout);
        return -1;
    }

    return 1;
}

char	*success_msg = "Starting kernel";
char	*err_msg1 = "error: connection not available";
char	*err_msg2 = "BOOTP broadcast 4";

/*----------------------------------------------------------
 * Powercycle_bgnd: powercycle a backend
 * --------------------------------------------------------*/
int powercycle_bgnd (char * class, char * connection, char * host)
{

	int	child;
	int	child1, child2;
	int	pipefd[2];
	int	fork_child1;
	char	conn[128];
	char	*str;
	size_t	size;
	FILE	*pfile = NULL;
	int	err;
	int	rv;

	fork_child1 = 1;

	sprintf(conn, "%s-pc", connection);

	while(1) {

		if(fork_child1) {

			rv = pipe2(pipefd, 0);
			if(rv == -1) {
				perror("pipe2(): ");
				exit(1);
			}

			child1 = fork();

			fork_child1 = 0;
		}

		if(child1 == 0) { /* Child */

			close(pipefd[0]);
			rv = dup2(pipefd[1], fileno(stdout));
			if(rv == -1) {
				perror("dup2(): ");
				exit(1);
			}

			rv = dup2(pipefd[1], fileno(stderr));
			if(rv == -1) {
				perror("dup2(): ");
				exit(1);
			}

			//printf("pcycle: opening connection to %s\n", connection);
			rv = execlp("cs-console", "cs-console", connection, NULL);
			if(rv == -1) {
				perror("execlp(): 1: ");
				return -1;
			}
		}
		else if(child1 > 0) { /* Parent */

			child2 = fork();

			if(child2 == 0) {

				rv = execlp("cs-console", "cs-console", "-t", "-f", "-c", "POWERCYCLE", "-s", host, conn, NULL);
				if(rv == -1) {
					perror("execlp(): 2: ");
					exit(1);
				}
			}
			else if(child2 > 0) {
				waitpid(child2, NULL, 0);
			}

			if(!pfile) {
				pfile = fdopen(pipefd[0], "r");
			}

			err = 1;
			while(1) {

				str = NULL;
				size = 0;

				getline(&str, &size, pfile);
				//printf("-%s", str);

				if(!strncmp(str, success_msg, strlen(success_msg))) {
					err = 0;
					break;
				}

				if(!strncmp(str, err_msg1, strlen(err_msg1))) {
					err = 1;
					kill(child1, 9);
					waitpid(child1, NULL, 0);
					fork_child1 = 1;
					close(pipefd[0]);
					close(pipefd[1]);
					fclose(pfile);
					pfile = NULL;
					break;
				}

				if(!strncmp(str, err_msg2, strlen(err_msg2))) {
					err = 1;
					break;
				}
			}

			if(err == 0) {
				close(pipefd[0]);
				close(pipefd[1]);
				fclose(pfile);
				//kill(child1, SIGKILL);
				//waitpid(child1, NULL, 0);
				//printf("child1 process ended. connection: %s\n", connection);
				sleep(1);
				break;
			}

			sleep(1);
		}
	}

	return 1;

#if 0
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
#endif
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


