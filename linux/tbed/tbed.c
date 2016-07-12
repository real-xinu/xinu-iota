#include "include/headers.h"
#include "include/prototypes.h"

const char *SRV_IP;
char map_list[46][100];
char map_serv[15];
char ip_serv[15];
char map_brouter[15];
/*--------------------------------------------------------
This function is used for handling errors
------------------------------------------------------*/
void error_handler (char *s)
{
    perror (s);
    exit (1);
}
/*--------------------------------------------------------------
 * Make a mapping list between nodes' name and nodes' ID
 * ------------------------------------------------------------*/
void mapping_list (char *fname)
{
    int i;
    //char name[100];
    unsigned char name_size[1];
    //char *path = "/homes/arastega/Wi-sun-repo/xinu-bbb/remote-file-server/";
    char *file_name = (char *) malloc (1 + strlen (path) + strlen (fname));
    strcpy (file_name, path);
    strcat (file_name, fname);
    FILE *fp;
    unsigned char nmcast[6];
    int zeros = 0;
    int flag = 0;
    int nnodes = 0;
    fp = fopen (file_name, "rb");

    if (fp == NULL) {
        printf ("Cannot open the file. \n");
        return;
    }

    memset (map_list, 0, sizeof (map_list));

    while (fread (nmcast, sizeof (nmcast), 1, fp) > 0) {
        for (i = 0; i < 6; i++) {
            if ((int)nmcast[i] == 0)
                zeros++;
        }

        if (zeros == 6) {
            flag = 1;
            break;

        } else
            nnodes++;

        zeros = 0;
    }

    //printf("number of nodes: %d\n",nnodes);
    i = 0;

    while (i < nnodes) {
        fread (name_size, sizeof (name_size), 1, fp);
        fread (map_list[i], (int) *name_size, 1, fp);
        //printf("name is: %s:%d\n" ,map_list[i], (int)*name_size);
        i++;
    }
}

/*---------------------------------------------------------
 * ISNUMERIC Function
 * -------------------------------------------------------*/
int isnumeric (char *str)
{
    int len = strlen (str);
    int i;

    for (i = 0; i < len; i++) {
        if ((int)str[i] < 48 || (int)str[i] > 57)
            return 0;
    }

    return 1;
}
/*----------------------------------------------------------
 This function is used to handle operator commands.
*------------------------------------------------------------*/
struct c_msg  command_handler (char command[BUFLEN])
{
    struct c_msg message;
    memset (&message, 0, sizeof (struct c_msg));
    char array_token[2][20];
    char seps[]   = " ";
    char *token;
    char beagle[10];
    int num;
    int i;
    int flag = 0;
    i = 0;
    token = strtok (command, seps );

    /* While there are tokens in "command" */
    while ( token != NULL ) {
        strcpy (array_token[i], token);
        token = strtok ( NULL, seps); /* Get next token: */
        i++;
    }

    if (!strcmp (array_token[0], "restart") && !strcmp (array_token[1], "t")) {
        message.cmsgtyp = htonl (C_RESTART);

    } else if (!strcmp (array_token[0], "xon")) {
        message.cmsgtyp = htonl (C_XON);
        num = atoi (array_token[1]);

        if (((num >= 0) && (num <= 45)) || (num == -1)) {
            message.xonoffid = htonl (num);

        } else {
            fprintf (fp, "Incorrect ID\n");
            message.cmsgtyp = htonl (C_ERR);
        }

    } else if (!strcmp (array_token[0], "xoff")) {
        message.cmsgtyp = htonl (C_XOFF);
        num = atoi (array_token[1]);

        if (((num >= 0) && (num <= 45)) || (num == -1)) {
            message.xonoffid = htonl (num);

        } else {
            fprintf (fp, "Incorrect ID\n");
            message.cmsgtyp = htonl (C_ERR);
        }

    } else if ((!strcmp (array_token[0], "nping")) && strcmp (array_token[1], " ") && strcmp (array_token[1], "all")) {
        message.cmsgtyp = htonl (C_PING_REQ);
        message.clength = htonl (sizeof (message));

        if (isnumeric (array_token[1]) == 1) {
            num = atoi (array_token[1]);

            if ((num >= 0) && (num <= 45)) {
                message.pingnodeid = htonl (num);

            } else {
                fprintf (fp, "Incorrect ID\n");
                message.cmsgtyp = htonl (C_ERR);
            }

        } else {
            for (i = 0; i < 46; i++) {
                if (! (strcmp (map_list[i], array_token[1]))) {
                    message.pingnodeid = htonl (i);
                    flag = 1;
                }
            }

            if (flag == 0) {
                fprintf (fp, "Incorrect Node Name\n");
                message.cmsgtyp = htonl (C_ERR);
            }
        }

    } else if ((!strcmp (array_token[0], "nping")) && strcmp (array_token[1], " ") && (!strcmp (array_token[1], "all"))) {
        message.cmsgtyp = htonl (C_PING_ALL);
        message.pingnodeid = htonl (ALL);

    } else if (!strcmp (array_token[0], "topdump")) {
        message.cmsgtyp = htonl (C_TOP_REQ);

    } else if (!strcmp (array_token[0], "newtop") && (strcmp (array_token[1], " "))) {
        message.cmsgtyp = htonl (C_NEW_TOP);
        message.flen = htonl (strlen (array_token[1]));
        strcpy ((char *)message.fname , array_token[1]);
        mapping_list ((char *)message.fname);

    } else if (!strcmp (array_token[0], "online")) {
        message.cmsgtyp = htonl (C_ONLINE);

    } else if (!strcmp (array_token[0], "offline")) {
        message.cmsgtyp = htonl (C_OFFLINE);

    } else if (!strcmp (array_token[0], "delay")) {
        int delay = atoi (array_token[1]);
        usleep (delay);
        message.cmsgtyp = htonl (C_ERR);

    } else if (!strcmp (array_token[0], "exit")) {
        fprintf (fp, "====Management app is closed=====\n");
        exit (1);

    } else if (!strcmp (array_token[0], "help")) {
    } else if (!strcmp (array_token[0], "ts_find")) {
        message.cmsgtyp = htonl (C_TS_REQ);

    } else if (!strcmp (array_token[0], "ts_check")) {
        message.cmsgtyp = htonl (C_TS_REQ);

    } else if (!strcmp (array_token[0], "download") && array_token[1] != NULL && array_token[2] != NULL) {
        if (!strcmp (array_token[1], "t")) {
            memset (beagle, 0, sizeof (beagle));
            strcpy (beagle, "beagle");
            strcat (beagle, array_token[2]);
            strcpy (map_serv, beagle);
            strcat (ip_serv, NETIP);
            strcat (ip_serv, array_token[2]);
            //printf("ip: %s", ip_serv);
            download_img (array_token[3], "cortex", beagle, XINUSERVER);
            message.cmsgtyp = htonl (C_ERR);

        } else if (!strcmp (array_token[1], "b")) {
            memset (beagle, 0, sizeof (beagle));
            strcpy (beagle, "beagle");
            strcat (beagle, array_token[2]);
            strcpy (map_brouter, beagle);
            download_img (array_token[3], "cortex", beagle, XINUSERVER);
            message.cmsgtyp = htonl (C_ERR);

        } else if (!strcmp (array_token[1], "n")) {
            memset (beagle, 0, sizeof (beagle));
            strcpy (beagle, "beagle");
            strcat (beagle, array_token[2]);
            download_img (array_token[3], "cortex", beagle, XINUSERVER);
            message.cmsgtyp = htonl (C_ERR);
        }

    } else if (!strcmp (array_token[0], "pcycle")) {
        if (!strcmp (array_token[1], "t")) {
            powercycle_bgnd ("cortex", map_serv, XINUSERVER);
            SRV_IP = ip_serv;

            if (inet_aton (SRV_IP, &si_other.sin_addr) == 0) {
                fprintf (stderr, "inet_aton() failed\n");
                exit (1);
            }

           
            message.cmsgtyp = htonl (C_ERR);

        } else if (atoi (array_token[1]) > 101 && atoi (array_token[1]) < 184) {
            memset (beagle, 0, sizeof (beagle));
            strcpy (beagle, "beagle");
            strcat (beagle, array_token[1]);
            powercycle_bgnd ("cortex", beagle, XINUSERVER);
            message.cmsgtyp = htonl (C_ERR);
        }

    } else {
        fprintf (fp, "%s is not defined\n", array_token[0]);
        message.cmsgtyp = htonl (C_ERR);
    }

    return message;
}



int download_img (char * filename, char * class, char * connection, char * host)
{
    pid_t pid_dwnld;
    pid_dwnld = fork();

    if (pid_dwnld == 0) {
        /* Child Process */
        char conn[128];
        int fd;

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

int connect_bgnd (char * class, char * connection, char * host)
{
    /* Make connection */
    int ret = makeConnection (connection, class, host);
    return ret;
}




/*-----------------------------------------------------
* Print the network topology based on control message
which have been recevied from the testbed server.
*-----------------------------------------------------*/
void topodump (struct c_msg *buf)
{
    fflush (stdout);

    if (ntohl (buf->topnum) == 0) {
        fprintf (fp, "Please enter newtop command\n");
    }

    if (ntohl (buf->topnum) > 0) {
        fprintf (fp, "Number of Nodes: %d\n", ntohl (buf->topnum));
    }

    int entries = ntohl (buf->topnum);
    byte mcaddr[6];

    for (int i = 0; i < entries; i++) {
        fprintf (fp, "%d ,", ntohl (buf->topdata[i].t_nodeid));
        fprintf (fp, "%d ,", ntohl (buf->topdata[i].t_status));

        for (int j = 0; j < 6; j++) {
            for (int k = 7; k >= 0; k--) {
                fprintf (fp, "%d", (buf->topdata[i].t_neighbors[j] >> k) & 0x01);
            }

            fprintf (fp, " ");
            mcaddr[j] = buf->topdata[i].t_neighbors[j];
        }

        fprintf (fp, "\nThe neighbors of node %d are: ", ntohl (buf->topdata[i].t_nodeid));

        for (int j = 0; j < 46; j++) {
            if (srbit (mcaddr, j, BIT_TEST) == 1) {
                fprintf (fp, "%d ", j);
            }
        }

        fprintf (fp, "\n");
    }
}


/*----------------------------------------------------------------------------
 *ts_find: is used to implement ts_find and ts_check commands.
 * -------------------------------------------------------------------------*/
int ts_find()
{
    char *recvbuf;
    recvbuf = malloc (sizeof (struct c_msg));
    struct sockaddr_in  si_other;
    socklen_t slen = sizeof (si_other);
    struct c_msg *buf = malloc (sizeof (struct c_msg));
    struct c_msg message;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIME_OUT_TS;
    char list_ip[15][46];
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons (PORT);

    if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) < 0) {
        error_handler ("socket Option for timeout can not be set");
    }

    if (inet_aton (IP_BRDCAST, &si_other.sin_addr) == 0) {
        fprintf (stderr, "inet_aton() failed\n");
        exit (1);
    }

    int bcast = 1;

    if (setsockopt (s, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof (bcast))) {
        error_handler ("Broadcast socket option cannot be set");
    }

    /* -----------------------------------------------------------
     * Create a C_TS_REQ message and send it as broadcast message
     *----------------------------------------------------------*/
    message.cmsgtyp = htonl (C_TS_REQ);

    if (sendto (s, &message, sizeof (message), 0 , (struct sockaddr *)&si_other, slen) == -1) {
        error_handler ("The message is not sent to Testbed_Server()");
    }

    fprintf (fp, "Looking for the Testbed Servers...\n");
    int i = 0, j = 0;

    /* -------------------------------------------------------------
     * Wait for 50 ms to collect responses from the testbed servers
     *-------------------------------------------------------------*/
    while (i < 50) {
        if ((recvfrom (s, recvbuf, sizeof (struct c_msg), 0, (struct sockaddr *)&si_other, &slen)) < 0) {
        }

        buf = (struct c_msg*)recvbuf;

        /*---------------------------------------------------------
        * Create a list of running servers' IPv4 addresses
         * -------------------------------------------------------*/
        if (ntohl (buf->cmsgtyp) == C_TS_RESP) {
            int k = 0;
            int flag = 0;

            for (k = 0; k < j; k++) {
                if (!strcmp (list_ip[k], inet_ntoa (si_other.sin_addr))) {
                    flag = 1;
                }
            }

            if (flag == 0) {
                strcpy (list_ip[j], inet_ntoa (si_other.sin_addr));
                j++;
            }
        }

        i++;
    }

    i = 0;

    /*------------------------------------------------------
     * print the list of IP addresses
     * -----------------------------------------------------*/
    for (i = 0; i < j; i++) {
        fprintf (fp, "IP address of Testbed server %d is :%s\n", i + 1, list_ip[i]);
    }

    tv.tv_sec = TIME_OUT;

    if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) < 0) {
        error_handler ("socket Option for timeout can not be set");
    }

    /*--------------------------------------------------------------
     * disable the broadcast option on UDP socket
     * -----------------------------------------------------------*/
    bcast = 0;

    if (setsockopt (s, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof (bcast))) {
        error_handler ("Broadcast socket option cannot be set");
    }

    return j;
}


/*-----------------------------------------------------------------------------------------------
 * ping reply control message which is received from testbed server is handled in this function
 * it prints the staus of a node or a set of nodes  that have been  pinged.
 -----------------------------------------------------------------------------------------------*/

void ping_reply_handler (struct c_msg *buf)
{
    int i, counter, status;
    counter = ntohl (buf->pingnum);

    for (i = 0; i < counter; i++) {
        status = ntohl (buf->pingdata[i].pstatus);

        if (status == ALIVE) {
            fprintf (fp, "<====Reply from testbed server: Node %d is alive\n", ntohl (buf->pingdata[i].pnodeid));

        } else if ((status  == NOTACTIV) && (counter == 1)) {
            fprintf (fp, "<====Reply from testbed server: Node %d is not in the active network topology\n", ntohl (buf->pingdata[i].pnodeid));

        } else if (status == NOTRESP) {
            fprintf (fp, "<====Reply from testbed server: Node %d is not responding \n", ntohl (buf->pingdata[i].pnodeid));
        }
    }
}

/*-----------------------------------------------------------*
 *  Control message response handler is used to handle
 *  responses from the testbed server
 *  ---------------------------------------------------------*/

void response_handler (struct c_msg *buf)
{
    int32 cmsgtyp = ntohl (buf->cmsgtyp);

    /*-------------------------------------------
     * Control message type should be checked and
     * an appropiate function should be called to
     * handle it
     * ------------------------------------------*/
    switch (cmsgtyp) {
        case C_OK:
            fprintf (fp, "<====Reply from testbed server: OK\n");
            break;

        case C_ERR:
            fprintf (fp, "<====Reply from testbed server:ERROR\n");
            break;

        case C_TOP_REPLY:
            topodump (buf);
            break;

        case C_PING_REPLY:
            ping_reply_handler (buf);
            break;

        case C_PING_ALL:
            ping_reply_handler (buf);
            break;
    }
}



/*------------------------------------------------------------------------
 * this UDP process is used to communicate with testbed server on port
 * 55000. The communication between managment app and testbed server
 * is based on control protocol.
 *------------------------------------------------------------------------*/

void udp_process (const char *SRV_IP, char *file)
{
    char *recvbuf;
    struct sockaddr_in si_me;//, si_other;
    //int s;
    int enable;
    socklen_t slen = sizeof (si_other);
    struct c_msg *buf;
    char command[BUFLEN];
    struct c_msg message;
    struct timeval tv;
    tv.tv_sec = TIME_OUT;
    tv.tv_usec = 0;
    FILE *type;

    /*-------------------------------------------------------------------------
     * Create a UDP socket on port 55000 to comuunicate with the testbed server
     * -------------------------------------------------------------------*/
    if ((s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        error_handler ("socket");
    }

    enable = 1;

    if (setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof (int)) < 0)
        error_handler ("setsockopt(SO_REUSEADDR) failed");

    if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) < 0) {
        error_handler ("socket Option for timeout can not be set");
    }

    memset ((char *)&si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons (PORT);
    si_me.sin_addr.s_addr = htonl (INADDR_ANY);

    if ( bind (s , (struct sockaddr*)&si_me, sizeof (si_me) ) == -1) {
        error_handler ("Sock can not bind to the address");
    }

    /*---------------------------------------------------------
     * Determine the testbed server's IP address
     * -------------------------------------------------------*/
    if (inet_aton (SRV_IP, &si_other.sin_addr) == 0) {
        fprintf (stderr, "inet_aton() failed\n");
        exit (1);
    }

    si_other.sin_family = AF_INET;
    si_other.sin_port = htons (PORT);

    if (file == NULL) {
        type = stdin;

        while (1) {
            /*-----------------------------------------------------------------
             * To receive command from command line
             * ---------------------------------------------------------------*/
            memset (command, 0, sizeof (char) * BUFLEN);
            printf ("\n(Enter Command)# ");
            fgets (command, sizeof (command), type);

            while (!strcmp (command, "\n")) {
                printf ("\n(Enter Command)# ");
                fgets (command, sizeof (command), type);
            }

            command[strcspn (command, "\r\n")] = 0;
            memset (&message, 0, sizeof (struct c_msg));
            /*--------------------------------------------------------------------
            * create an appropiate control message and send to the testbed server
            * ------------------------------------------------------------------*/
            message = command_handler (command);

            if (message.cmsgtyp == htonl (C_TS_REQ) && !strcmp (command, "ts_find")) {
                ts_find();
            }

            if (message.cmsgtyp == htonl (C_TS_REQ) &&  !strcmp (command, "ts_check")) {
                int ntserver = ts_find();

                if (ntserver > 1) {
                    fprintf (fp, "More than one server is running\n");
                    close (s);
                    exit (1);
                }
            }

            /*--------------------------------------------------------------------------------
            * Send the messages to the testbed server
            * -------------------------------------------------------------------------------*/
            if (message.cmsgtyp != htonl (C_ERR) && message.cmsgtyp != htonl (C_TS_REQ)) {
                if (sendto (s, &message, sizeof (message), 0 , (struct sockaddr *)&si_other, slen) == -1) {
                    error_handler ("The message is not sent to Testbed_Server()");
                }

                recvbuf = malloc (sizeof (struct c_msg));

                if ((recvfrom (s, recvbuf, sizeof (struct c_msg), 0, (struct sockaddr *)&si_other, &slen)) < 0) {
                    fprintf (fp, "Timeout, Please try again\n");
                }

                buf = malloc (sizeof (struct c_msg));
                buf = (struct c_msg*)recvbuf;
                response_handler (buf);
                free (buf);
            }
        }

    } else {
        /*---------------------------------------------------------------
        * To receive command from a script of commands
         * --------------------------------------------------------------*/
        type = fopen (file, "r");

        while ( fgets (command, sizeof (command), type) != NULL) {
            command[strcspn (command, "\r\n")] = 0;
            /*-----------------------------------------------------------------------
             * create an appropiate control message to send to the testbed server
             * ---------------------------------------------------------------------*/
            message = command_handler (command);

            if (message.cmsgtyp == htonl (C_TS_REQ) && !strcmp (command, "ts_find")) {
                ts_find();
            }

            if (message.cmsgtyp == htonl (C_TS_REQ) &&  !strcmp (command, "ts_check")) {
                int ntserver = ts_find();

                if (ntserver > 1) {
                    fprintf (fp, "More than one server is running\n");
                    close (s);
                    exit (1);
                }
            }

            if (message.cmsgtyp != htonl (C_ERR) && message.cmsgtyp != htonl (C_TS_REQ)) {
                if (sendto (s, &message, sizeof (message), 0 , (struct sockaddr *)&si_other, slen) == -1) {
                    error_handler ("The message is not sent to Testbed_Server()");
                }

                recvbuf = malloc (sizeof (struct c_msg));

                if ((recvfrom (s, recvbuf, sizeof (struct c_msg), 0, (struct sockaddr *)&si_other, &slen)) < 0) {
                    fprintf (fp, "Timeout, Please try again\n");
                }

                buf = (struct c_msg*)recvbuf;
                response_handler (buf);
                free (buf);
            }
        }
    }

    close (s);
}

/* Main- Call UDP process */
int main (int argc, char **argv)
{
    struct timeval  tv1, tv2;
    gettimeofday (&tv1, NULL);
    char  use[] = "error: use is tbed (  | <script> log | <script> stdout )\n";

    if ( (argc != 1)  && (argc != 3) ) {
        fprintf (stderr, "%s", use);
        exit (1);
    }

    if (argv[2] != NULL) {
        if (!strcmp ("stdout", argv[2])) {
            fp = stdout;

        } else if (!strcmp ("log", argv[2] )) {
            fp = fopen ("log.txt", "w");
        }

        SRV_IP = "0.0.0.0";
        udp_process (SRV_IP, argv[1]);

    } else {
        fp = stdout;
        SRV_IP = "0.0.0.0";
        udp_process (SRV_IP, argv[1]);
    }

    gettimeofday (&tv2, NULL);
    fprintf (fp, "Emulation time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));
    return 0;
}
