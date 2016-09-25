#include "../include/headers.h"
#include "../include/prototypes.h"
#include "../include/macros.h"
#include "../include/global.h"




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
    unsigned int uptime[15]; 
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons (PORT);

    if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) < 0) {
        error_handler ("socket Option for timeout can not be set");
    }

    /* ---------------------------------------------------
     * Set broadcast IP address
     * ---------------------------------------------------*/
    if (inet_aton (IP_BRDCAST, &si_other.sin_addr) == 0) {
        fprintf (stderr, "inet_aton() failed\n");
        exit (1);
    }

    /* --------------------------------------
     * Enable broadcast option on the socket
     * -------------------------------------*/
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
		uptime[j] = ntohl(buf->uptime);
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
        fprintf (fp, "IP address of Testbed server %d is :%s, and it is running for %d seconds\n", i + 1, list_ip[i], uptime[i]);
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


