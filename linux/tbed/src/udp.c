#include "../include/headers.h"
#include "../include/prototypes.h"
#include "../include/macros.h"
#include "../include/global.h"

//char *path = "../../../remote-file-server/";
char *scripts_path = "../scripts/";
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
    FILE *inc, *inc2;

    /*-------------------------------------------------------------------------
    Create a UDP socket on port 55000 to comuunicate with the testbed server
    ---------------------------------------------------------------------------*/
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
            printf ("\nwsh$ ");
            fgets (command, sizeof (command), type);

            while (!strcmp (command, "\n")) {
                printf ("\nwsh$ ");
                fgets (command, sizeof (command), type);
            }

            //printf ("here");
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

        //printf("command:%s\n", command);
        while ( fgets (command, sizeof (command), type) != NULL) {
            command[strcspn (command, "\r\n")] = 0;
            /*-----------------------------------------------------------------------
             * create an appropiate control message to send to the testbed server
             * ---------------------------------------------------------------------*/
            message = command_handler (command);

            if ((!strcmp (array_token[0], "inc")) && message.cmsgtyp == htonl (C_ERR)) {
                char *temp_path = (char *)malloc (1 + strlen (scripts_path) + strlen (array_token[1]));
                strcpy (temp_path, scripts_path);
                strcat (temp_path, array_token[1]);
                /*DEBUG */ // printf("%s\n", temp_path);
                inc = fopen (temp_path, "r");
                free (temp_path);

                while (fgets (command, sizeof (command), inc) != NULL) {
                    /*DEBUG */ //fprintf(fp, "%s\n", command);
                    command[strcspn (command, "\r\n")] = 0;
                    message = command_handler (command);

                    if ((!strcmp (array_token[0], "inc")) && message.cmsgtyp == htonl (C_ERR)) {
                        temp_path = (char *)malloc (1 + strlen (scripts_path) + strlen (array_token[1]));
                        strcpy (temp_path, scripts_path);
                        strcat (temp_path, array_token[1]);
                        inc2 = fopen (temp_path, "r");

                        while (fgets (command, sizeof (command), inc2) != NULL) {
                            command[strcspn (command, "\r\n")] = 0;
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

                    } else {
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

            } else {
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
    }

    close (s);
}



