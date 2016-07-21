#define IP_BRDCAST   "128.10.137.255"
#define BUFLEN 2048
#define PORT 55000  // UDP prort
#define ERR -1
#define OK 1
#define BIT_TEST 1
#define BIT_SET 0
#define TIME_OUT 10
#define TIME_OUT_TS 1000
#define ALL -1     // This macro is used for ping all
// toptology status variables
#define ALIVE 1
#define NOTRESP -1
#define NOTACTIV 0

//cs-console commands
#define DOWNLOAD "DOWNLOAD"
#define POWERCYCLE "POWERCYCLE"
#define EXEC "cs-console"
#define SERVERIMG "xinu.boot.s"
#define NODEIMG   "xinu.boot.n"
#define XINUSERVER "xinuserver.cs.purdue.edu"
#define SERVER "beagle183"
#define NETIP   "128.10.137."

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include "control-protocol.h"
#include "compatibility.h"
#include <time.h>
#include <fcntl.h>
#include "console_connect.h"
 int s;
FILE *fp;
char *path = "/homes/arastega/Wi-sun-repo/xinu-bbb/remote-file-server/";
char *imgpath = "../images/";
extern int srbit (byte addr[], int, int);
extern int isnumeric(char *);
struct sockaddr_in si_other;
extern int makeConnection(char *, char *, char *);
const char *SRV_IP;
char map_list[46][100];
char map_serv[15];
char ip_serv[15];
char map_brouter[15];




const char * const  cmdtab[] = {
   "cleanup",
   "delay",
   "download",
   "inc",
   "newtop",
   "nping",
   "offline",
   "online",
   "pcycle",
   "restart",
   "tcp",
   "topdump",
   "ts_1",
   "ts_check",
   "ts_find",
   "tshutdown",
   "udp",
   "xoff",
   "xon"

};

int ncmd = sizeof(cmdtab)/ sizeof(const char *);

