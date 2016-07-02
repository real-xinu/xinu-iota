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


extern int srbit (byte addr[], int, int);


