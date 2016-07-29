#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define DOWNLOAD "DOWNLOAD"
#define POWERCYCLE "POWERCYCLE"
#define EXEC "cs-console"

/* gcc -o console console_connect.c -I ./ConsoleTools/hdr ./ConsoleTools/lib/*.c */

int download_img(char * filename, char * class, char * connection, char * host)
{
	pid_t pid_dwnld;

	pid_dwnld = fork();
        printf("id:%d\n",pid_dwnld);
	if(pid_dwnld == 0)
	{
		/* Child Process */
		char conn[128];

		int fd;
		
		if( ( fd = open(filename, O_RDONLY ) ) < 0 ) 
		{
			perror( "open()" );
			exit( 1 );
		}
                printf("hello\n");		
		if( dup2( fd, 0 ) < 0 ) 
		{
			perror( "dup2()" );
			exit( 1 );
		}
		close( fd );

		sprintf(conn, "%s%s", connection, "-dl");
                
		execlp(EXEC, EXEC, "-t", "-f", "-c", DOWNLOAD, "-s", host, conn, NULL);
		fprintf(stderr, "execlp() failes\n");
		return -1;
	}
	else if(pid_dwnld < 0)
	{
		fprintf(stdout, "\nfork() Error\n");
		return -1;
	}
}

int powercycle_bgnd(char * class, char * connection, char * host)
{
	pid_t pid_dwnld;

	pid_dwnld = fork();

	/* Child process to execute cs-console command */
	if(pid_dwnld == 0)
	{
		/* Child Process */
		char conn[128];

		sprintf(conn, "%s%s", connection, "-pc");

		execlp(EXEC, EXEC, "-t", "-f", "-c", POWERCYCLE, "-s", host, conn, NULL);
		fprintf(stderr, "execlp() failes\n");
		return -1;
	}
	else if(pid_dwnld < 0)
	{
		fprintf(stdout, "\nfork() Error\n");
		return -1;
	}
}

int connect_bgnd(char * class, char * connection, char * host)
{
	/* Make connection */
	int ret = makeConnection(connection, class, host);

	return ret;
}

int main(int argc, char ** argv)
{
	/* Connect, download and powercycle */
	int ret = 0;
	if( (ret = connect_bgnd("cortex", "beagle114", "xinuserver.cs.purdue.edu")) < 0 )
		exit(-1);
	//fprintf(stderr, "%d\n", ret);
	download_img("xinu.boot.n", "cortex", "beagle114", "xinuserver.cs.purdue.edu");
	//powercycle_bgnd("cortex", "beagle114", "xinuserver.cs.purdue.edu");
}
