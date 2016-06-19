#include "headers.h"

//#define SRV_IP   "128.10.136.187"
#define BUFLEN 2048
#define PORT 55000
#define ERR -1
#define BIT_TEST 1
#define BIT_SET 0
#define TIME_OUT 10

extern int srbit(byte addr[], int, int);

//This function is used for handling errors
void error_handler(char *s)
{
    perror(s);
    exit(1);

}
/*******************************************************
 This function is used to handle operator commands.

*******************************************************/
struct c_msg  command_handler(char command[BUFLEN])
{
    struct c_msg message;
    char array_token[2][20];
    char seps[]   = " ";
    char *token;
    int num;
    int i;


    i = 0;
    token = strtok(command, seps );
    while( token != NULL )
    {
        /* While there are tokens in "command" */
        strcpy(array_token[i], token);
        /* Get next token: */
        token = strtok( NULL, seps);
        i++;
    }

    if (!strcmp(array_token[0], "restart"))
    {
        message.cmsgtyp = htonl(C_RESTART);
    }
    
    else if(!strcmp(array_token[0], "xon"))
    {
        message.cmsgtyp = htonl(C_XON);
        num = atoi(array_token[1]);
        if (((num >= 0) && (num <= 45)) || (num == -1))
        {
            message.xonoffid = htonl(num);
        }
        else
        {

            printf("Incorrect ID\n");
            message.cmsgtyp = htonl(ERR);
        }

    }
    else if (!strcmp(array_token[0], "xoff"))
    {
        message.cmsgtyp = htonl(C_XOFF);
        num = atoi(array_token[1]);
        if (((num >= 0) && (num <= 45)) || (num == -1))
        {
            message.xonoffid = htonl(num);
        }
        else
        {

            printf("Incorrect ID\n");
            message.cmsgtyp = htonl(ERR);
        }
    }
    
    
    else if(!strcmp(array_token[0], "nping"))
    {
        message.cmsgtyp = htonl(C_PING_REQ);
        num = atoi(array_token[1]);
        if ((num >= 0) && (num <= 45))
        {
            message.pingnodeid = htonl(num);
        }
        else
        {
            printf("Incorrect ID\n");
            message.cmsgtyp = htonl(ERR);
        }
    }
    
    else if(!strcmp(array_token[0], "topdump"))
    {
        message.cmsgtyp = htonl(C_TOP_REQ);
    }
    else if (!strcmp(array_token[0], "newtopo"))
    {
        message.cmsgtyp = htonl(C_NEW_TOP);
        strcpy((char *)message.fname ,array_token[1]);
    }
    else if (!strcmp(array_token[0], "online"))
    {


    }
    else if(!strcmp(array_token[0], "offline"))
    {

    

    }
    else if (!strcmp(array_token[0], "exit"))
    {
        exit(1);
    }
    else if(!strcmp(array_token[0], "help"))
    {

        message.cmsgtyp = htonl(ERR);


    }
    else
    {
        printf("%s is not defined\n", array_token[0]);
        message.cmsgtyp = htonl(ERR);

    }

    return message;

}
/******************************************************
* Print the network topology based on control message
which have been recevied from the testbed server.
******************************************************/


void print_topology(struct c_msg *buf)
{
    fflush(stdout);
    printf("Number of Nodes: %d\n",ntohl(buf->topnum));
    int entries = ntohl(buf->topnum);
    byte mcaddr[6];
    for (int i=0; i<entries; i++)
    {
        printf("%d ,", ntohl(buf->topdata[i].t_nodeid));
        printf("%d ,",ntohl(buf->topdata[i].t_status));
        for (int j=0; j <6; j++)
        {
            
                  for (int k=7; k>=0; k--)
                  {
                      printf("%d", (buf->topdata[i].t_neighbors[j]>>k)&0x01);
                  }
		  printf(" ");
            //printf("%d ",buf->topdata[i].t_neighbors[j]);
            mcaddr[j] = buf->topdata[i].t_neighbors[j];

        }
        printf("\nThe neighbors of Node %d are: ",ntohl(buf->topdata[i].t_nodeid));
        for (int j=0; j<46; j++)
        {
            if (srbit(mcaddr, j, BIT_TEST) == 1)
            {
                printf("%d ",j);
            }
        }
        printf("\n");
    }
}

/*********************************************************
 *
 *  This function is used to discover the testbed server
 *
 ******************************************************/
int server_discovery(const char *SRV_IP)
{

    char *recvbuf;
    recvbuf = malloc(sizeof(struct c_msg));
    struct sockaddr_in si_me, si_other;
    int s;
    socklen_t slen = sizeof(si_other);
    struct c_msg *buf = malloc(sizeof(struct c_msg));
    struct c_msg message;
    struct timeval tv;
    tv.tv_sec = TIME_OUT;
    tv.tv_usec = 0;


    if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)  /* Create a UDP socket */
    {
        error_handler("socket");

    }


    memset((char *)&si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        error_handler("Sock can not bind to the address");


    }
    if (inet_aton(SRV_IP, &si_other.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)   /* Set a timeout for the cases that the management server does not
	                                                              receive any response from the testbed server */
    {
        error_handler("socket Option for timeout can not be set");
    }

    /* Ensure that we can broadcast on our socket */
    int bcast = 1;
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast)))
    {
        error_handler("Broadcast socket option cannot be set");
    }


    while(1)
    {

        message.cmsgtyp = htonl(C_TS_REQ);

        if(sendto(s, &message, sizeof(message), 0 , (struct sockaddr *)&si_other, slen) == -1)
        {
            error_handler("The message is not sent to Testbed_Server()");

        }
        printf("Looking for the Testbed Server...\n");


        if ((recvfrom(s,recvbuf, sizeof(struct c_msg), 0, (struct sockaddr *)&si_other,&slen)) < 0)
        {
            printf("Timeout, Please try again\n");
        }
        buf = (struct c_msg*)recvbuf;
        if(ntohl(buf->cmsgtyp) == C_TS_RESP)
        {
            printf("Reply from Testbed_Server, Testbed server is discovered.\n");
            close(s);
            return 1;
        }
        else
        {
            close(s);
            return 0;
        }
    }

}


void udp_process(const char *SRV_IP)
{

    char *recvbuf;
    recvbuf = malloc(sizeof(struct c_msg));
    struct sockaddr_in si_me, si_other;
    int s;
    socklen_t slen = sizeof(si_other);
    struct c_msg *buf = malloc(sizeof(struct c_msg));
    char command[BUFLEN];
    struct c_msg message;
    struct timeval tv;
    tv.tv_sec = TIME_OUT;
    tv.tv_usec = 0;



    if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)  /* Create a UDP socket */
    {
        error_handler("socket");

    }
    memset((char *)&si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        error_handler("Sock can not bind to the address");


    }
    if (inet_aton(SRV_IP, &si_other.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)   /* Set a timeout for the cases that the management server does not
	                                                              receive any response from the testbed server */
    {
        error_handler("socket Option for timeout can not be set");
    }


    while(1)
    {

        memset(command, '\0', sizeof(char) * BUFLEN);
        printf("\n(Enter Command): ");                               /*receives command from the operator */
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\r\n")] = 0;
        message = command_handler(command);                     /*create an appropiate control message to send to the testbed server */

        if (message.cmsgtyp != htonl(ERR))
        {
            if(sendto(s, &message, sizeof(message), 0 , (struct sockaddr *)&si_other, slen) == -1)
            {
                error_handler("The message is not sent to Testbed_Server()");

            }


            if ((recvfrom(s,recvbuf, sizeof(struct c_msg), 0, (struct sockaddr *)&si_other,&slen)) < 0)
            {
                printf("Timeout, Please try again\n");
            }
            buf = (struct c_msg*)recvbuf;

            if(ntohl(buf->cmsgtyp) == C_OK)
            {
                printf("Reply from Testbed_Server: %s\n", "OK");
            }
            else if(ntohl(buf->cmsgtyp) == C_TOP_REPLY)
            {

                print_topology(buf);
            }
            else if (ntohl(buf->cmsgtyp) == C_ERR)
            {

            }
        }

    }

    close(s);

}

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("Usage: %s <ip address of testbed server>\n", argv[0]);
        return ERR;

    }
    const char *SRV_IP = argv[1];

    if (server_discovery(SRV_IP) == 1)
        udp_process(SRV_IP);
    else
        printf("The Testbed server is not discovered.\n");


    return 0;


}
