#include "headers.h"
#include "prototypes.h"


/*--------------------------------------------------------
This function is used for handling errors
------------------------------------------------------*/
void error_handler (char *s)
{
    perror (s);
    exit (1);
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
    int num;
    int i;
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
            printf ("Incorrect ID\n");
            message.cmsgtyp = htonl (ERR);
        }

    } else if (!strcmp (array_token[0], "xoff")) {
        message.cmsgtyp = htonl (C_XOFF);
        num = atoi (array_token[1]);

        if (((num >= 0) && (num <= 45)) || (num == -1)) {
            message.xonoffid = htonl (num);

        } else {
            printf ("Incorrect ID\n");
            message.cmsgtyp = htonl (ERR);
        }

    } else if ((!strcmp (array_token[0], "nping")) && strcmp (array_token[1], " ") && strcmp (array_token[1], "all")) {
        message.cmsgtyp = htonl (C_PING_REQ);
        message.clength = htonl (sizeof (message));
        num = atoi (array_token[1]);

        if ((num >= 0) && (num <= 45)) {
            message.pingnodeid = htonl (num);

        } else {
            printf ("Incorrect ID\n");
            message.cmsgtyp = htonl (ERR);
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

    } else if (!strcmp (array_token[0], "online")) {
        message.cmsgtyp = htonl (C_ONLINE);

    } else if (!strcmp (array_token[0], "offline")) {
        message.cmsgtyp = htonl (C_OFFLINE);

    } else if (!strcmp (array_token[0], "delay")) {
        int delay = atoi (array_token[1]);
        usleep (delay);
        message.cmsgtyp = htonl (C_ERR);

    } else if (!strcmp (array_token[0], "exit")) {
        printf ("====Management app is closed=====\n");
        exit (1);

    } else if (!strcmp (array_token[0], "help")) {
    } else if (!strcmp (array_token[0], "ts_find")) {
        message.cmsgtyp = htonl (C_TS_REQ);

    } else if (!strcmp (array_token[0], "ts_check")) {
        //int ts_count = ts_find();
        //printf ("ts_count:%d\n", ts_count);
        //if (ts_count > 1) {
        //   error_handler ("More than one testbed server is running\n");
        // }
    } else {
        printf ("%s is not defined\n", array_token[0]);
        message.cmsgtyp = htonl (C_ERR);
    }

    return message;
}
/******************************************************
* Print the network topology based on control message
which have been recevied from the testbed server.
******************************************************/


void topodump (struct c_msg *buf)
{
    fflush (stdout);

    if (ntohl (buf->topnum) == 0) {
        printf ("Please enter newtop command\n");
    }

    if (ntohl (buf->topnum) > 0) {
        printf ("Number of Nodes: %d\n", ntohl (buf->topnum));
    }

    int entries = ntohl (buf->topnum);
    byte mcaddr[6];

    for (int i = 0; i < entries; i++) {
        printf ("%d ,", ntohl (buf->topdata[i].t_nodeid));
        printf ("%d ,", ntohl (buf->topdata[i].t_status));

        for (int j = 0; j < 6; j++) {
            for (int k = 7; k >= 0; k--) {
                printf ("%d", (buf->topdata[i].t_neighbors[j] >> k) & 0x01);
            }

            printf (" ");
            mcaddr[j] = buf->topdata[i].t_neighbors[j];
        }

        printf ("\nThe neighbors of node %d are: ", ntohl (buf->topdata[i].t_nodeid));

        for (int j = 0; j < 46; j++) {
            if (srbit (mcaddr, j, BIT_TEST) == 1) {
                printf ("%d ", j);
            }
        }

        printf ("\n");
    }
}



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

    message.cmsgtyp = htonl (C_TS_REQ);

    if (sendto (s, &message, sizeof (message), 0 , (struct sockaddr *)&si_other, slen) == -1) {
        error_handler ("The message is not sent to Testbed_Server()");
    }

    printf ("Looking for the Testbed Servers...\n");
    int i = 0, j = 0;

    while (i < 50) {
        if ((recvfrom (s, recvbuf, sizeof (struct c_msg), 0, (struct sockaddr *)&si_other, &slen)) < 0) {
        }

        buf = (struct c_msg*)recvbuf;

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

    for (i = 0; i < j; i++) {
        printf ("IP address of Testbed server %d is :%s\n", i + 1, list_ip[i]);
    }

    tv.tv_sec = TIME_OUT;

    if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) < 0) {
        error_handler ("socket Option for timeout can not be set");
    }

    bcast = 0;

    if (setsockopt (s, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof (bcast))) {
        error_handler ("Broadcast socket option cannot be set");
    }

    //close (s);
    return j;
}


/*-----------------------------------------------------
 * ping reply control message which is received from
 * testbed server is handled in this function
 * it prints the staus of a node or a set of nodes
 * that have been  pinged
 -----------------------------------------------------*/

void ping_reply_handler (struct c_msg *buf)
{
    int i, counter, status;
    counter = ntohl (buf->pingnum);

    for (i = 0; i < counter; i++) {
        status = ntohl (buf->pingdata[i].pstatus);

        if (status == ALIVE) {
            printf ("<====Reply from testbed server: Node %d is alive\n", ntohl (buf->pingdata[i].pnodeid));

        } else if ((status  == NOTACTIV) && (counter == 1)) {
            printf ("<====Reply from testbed server: Node %d is not in the active network topology\n", ntohl (buf->pingdata[i].pnodeid));

        } else if (status == NOTRESP) {
            printf ("<====Reply from testbed server: Node %d is not responding \n", ntohl (buf->pingdata[i].pnodeid));
        }
    }
}

/*-----------------------------------------------------------
 *
 *  Control message response handler is used to handle
 *  responses from the testbed server
 *  ---------------------------------------------------------*/

void response_handler (struct c_msg *buf)
{
    int32 cmsgtyp = ntohl (buf->cmsgtyp);

    switch (cmsgtyp) {
        case C_OK:
            printf ("<====Reply from testbed server: OK\n");
            break;

        case C_ERR:
            printf ("<====Reply from testbed server:ERROR\n");
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

        case C_TS_RESP:
            break;
    }
}



/*------------------------------------------------------------------------
 * this UDP process is used to communicate with testbed server
 *------------------------------------------------------------------------*/

void udp_process (const char *SRV_IP, char *file)
{
    char *recvbuf;
    struct sockaddr_in si_me, si_other;
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

    // int s = init_sock(SRV_IP);
    if ((s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) { // Create a UDP socket
        error_handler ("socket");
    }

    enable = 1;

    if (setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof (int)) < 0)
        error_handler ("setsockopt(SO_REUSEADDR) failed");

    if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) < 0)   // Set a timeout for the cases that the management server does not
        //receive any response from the testbed server
    {
        error_handler ("socket Option for timeout can not be set");
    }

    memset ((char *)&si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons (PORT);
    si_me.sin_addr.s_addr = htonl (INADDR_ANY);

    if ( bind (s , (struct sockaddr*)&si_me, sizeof (si_me) ) == -1) {
        error_handler ("Sock can not bind to the address");
    }

    if (inet_aton (SRV_IP, &si_other.sin_addr) == 0) {
        fprintf (stderr, "inet_aton() failed\n");
        exit (1);
    }

    si_other.sin_family = AF_INET;
    si_other.sin_port = htons (PORT);

    if (file == NULL) {
        type = stdin;

        while (1) {
            memset (command, 0, sizeof (char) * BUFLEN);
            printf ("\n(Enter Command)# ");                              /*receives command from the operator */
            fgets (command, sizeof (command), type);

            while (!strcmp (command, "\n")) {
                printf ("\n(Enter Command)# ");                              /*receives command from the operator */
                fgets (command, sizeof (command), type);
            }

            command[strcspn (command, "\r\n")] = 0;
            memset (&message, 0, sizeof (struct c_msg));
            message = command_handler (command);                    /*create an appropiate control message to send to the testbed server */

            if (message.cmsgtyp == htonl (C_TS_REQ)) {
                ts_find();
            }

            if (message.cmsgtyp != htonl (C_ERR) && message.cmsgtyp != htonl (C_TS_REQ)) {
                if (sendto (s, &message, sizeof (message), 0 , (struct sockaddr *)&si_other, slen) == -1) {
                    error_handler ("The message is not sent to Testbed_Server()");
                }

                recvbuf = malloc (sizeof (struct c_msg));

                if ((recvfrom (s, recvbuf, sizeof (struct c_msg), 0, (struct sockaddr *)&si_other, &slen)) < 0) {
                    printf ("Timeout, Please try again\n");
                }

                buf = malloc (sizeof (struct c_msg));
                buf = (struct c_msg*)recvbuf;
                response_handler (buf);
                free (buf);
            }
        }

    } else {
        type = fopen (file, "r");

        while ( fgets (command, sizeof (command), type) != NULL) {
            command[strcspn (command, "\r\n")] = 0;
            message = command_handler (command);                    /*create an appropiate control message to send to the testbed server */

            if (message.cmsgtyp == htonl (C_TS_REQ)) {
                ts_find();
            }

            if (message.cmsgtyp != htonl (C_ERR) && message.cmsgtyp != htonl (C_TS_REQ)) {
                if (sendto (s, &message, sizeof (message), 0 , (struct sockaddr *)&si_other, slen) == -1) {
                    error_handler ("The message is not sent to Testbed_Server()");
                }

                recvbuf = malloc (sizeof (struct c_msg));

                if ((recvfrom (s, recvbuf, sizeof (struct c_msg), 0, (struct sockaddr *)&si_other, &slen)) < 0) {
                    printf ("Timeout, Please try again\n");
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
    const char *SRV_IP = argv[1];
    udp_process (SRV_IP, argv[2]);
    return 0;
}
