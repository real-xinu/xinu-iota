/************************************************************************/
/*									*/
/*			Control Message Protocol			*/
/*									*/
/* The following defines the protocol used between the testbed		*/
/*	management system and the testbed server.  Each message is	*/
/*	sent in a UDP packet to port 55000.  All integer values	are	*/
/*	sent in network byte order.					*/
/*									*/
/************************************************************************/

/* Definition of message types */

#define MAX_BBB         84
#define File_Name_Size  50

#define	C_RESTART	0	/* A message sent to the testbed server	*/
/*  that requests the server to restart.*/

#define	C_RESTART_NODES	1	/* A message sent to the testbed server	*/
/*  that requests the server to restart	*/
/*  all the active nodes in the testbed.*/

#define	C_XOFF		2	/* A message sent to the testbed server	*/
/*  that requests the server to stop	*/
/*  a specific node or stop all activity*/
/*  in the testbed.			*/


#define	C_XON		3	/* A message sent to the testbed server	*/
/*  that requests the server to resume	*/
/*  activity on a single node or in the	*/
/*  entire testbed.			*/


#define	C_OFFLINE	4	/* A message sent to the testbed server	*/
/*  to request the server to cease all	*/
/*  interaction with nodes.  An OFFLINE	*/
/*  message might be sent before a	*/
/*  topology update.			*/


#define	C_ONLINE	5	/* A message sent to the testbed server	*/
/*  to request the server to resume	*/
/*  interaction with nodes.		*/


#define	C_PING_REQ	6	/* A message sent to the testbed server	*/
/*  that requests the server to ping a	*/
/*  specified node and return the	*/
/*  status.				*/

#define	C_PING_ALL	7	/* A message sent to the testbed server	*/
/*  that requests the server to ping	*/
/*  each node and return the status	*/

#define	C_PING_REPLY	8	/* A message sent from the testbed	*/
/*  server to the management system in	*/
/*  response to a C_PING_REQ or a	*/
/*  C_PING_ALL.  The possible answers	*/
/*  are: node is alive, node does not	*/
/*  respond, or node is	not in the	*/
/*  active topology.			*/

#define	C_TOP_REQ	9	/* A message sent to the testbed server	*/
/*  to request a dump of the topology.	*/

#define	C_TOP_REPLY	10	/* A message sent be the testbed server	*/
/*  with a response to a C_TOP_REQ.  A	*/
/*  resposne is in the same format as 	*/
/*  the topology database that the	*/
/*  server reads from a topology file.	*/

#define C_NEW_TOP	11	/* A message sent to the testbed server	*/
/*  to change the testbed topology. The	*/
/*  message contains the name of a file	*/
/*  that the server reads (using the	*/
/*  remote file system.*/
/*  If the name of the new file is in	*/
/*  the form xxx.N and the old file name*/
/*  is xxx.K for K < N, the server will	*/
/*  compare the old and new topologies,	*/
/*  and will only send changes to the	*/
/*  affected nodes.  If the file name	*/
/*  prefix changes, the server will	*/
/*  stop the existing test, and	 restart*/
/*  the testbed with the new topology.	*/

#define	C_TS_REQ	12	/* A query that the management system	*/
/*  broadcasts to find the testbed	*/
/*  server.  After it broadcasts a	*/
/*  C_TS_REQ message, the managmenet	*/
/*  system waits for 50 msec while it	*/
/*  collects responses.  If more than	*/
/*  one testbed server responds, the	*/
/*  management system sends a C_SHUTDOWN*/
/*  to all but one of the running	*/
/*  servers, and informs the user.	*/

#define	C_TS_RESP	13	/* A message sent by a testbed server	*/
/*  in response to a C_TS_REQ.  The	*/
/*  response contains amount of time in	*/
/*  msec since the testbed server booted*/
/*  (just to help with debugging).	*/

#define	C_SHUTDOWN	14	/* A message sent by the management	*/
/*  system to shut down a testbed server*/
/*  in a situation where more than one	*/
/*  testbed server is discovered (see	*/
/*  the C_FIND message).		*/

#define C_MAP           15  /* A message sent by the management system to obtain 
			       the mapping between the nodes' name and the IDs 
			       of the current network topology */

#define C_MAP_REPLY     16  /* a response to C_MAP message */

#define C_PINGALL  17   /* A message sent by the management system to 
			       ping all of the backends which are running 
			       a node image (including the nodes that are not in the current active
			       topology)*/

#define C_PINGALL_REPLY  18

#define	C_OK		19	/*  A response to an C_RESTART,		*/
/*  C_RESTART_NODES, C_XOFF, C_XON,	*/
/*  C_OFFLINE, C_ONLINE, C_NEW_TOP, or	*/
/*  C_SHUTDOWN that indicates the	*/
/*  request was honored and the		*/
/*  requested operation was successful.	*/

#define	C_ERR		20	/* A response sent by a node when an	*/
/*  incoming message is invalid or the	*/
/*  operation cannot be performed.	*/

#define C_CLEANUP       21      /* A message sent by management app to testbed
				   server to shut down all of the nodes */

#define C_SETTIME       22      /* A message sent by management app to testbed server 
                                   to set current time */

#define	C_MONITOR	23
/* The following struct is included here, but should probably go in the	*/
/*	file that declares items related to the topology database.	*/

#define	MAXNODES	46	/* Maximum number of nodes being tested	*/
#define NAMELEN         20      /* Maximum length of the node's name */

struct	topo_entry { 		/* Entry in a topology file (also used	*/
    /*  in messages.			*/
    int32	t_nodeid;	/* ID of a node				*/
    int32	t_status;	/* Status of the node			*/
    byte	t_neighbors[6];	/* The multicast address of neighbors	*/
    byte	t_macaddr[6];	/* The Mac address of a node		*/
    int32       t_bbbid;
    struct {
	          byte lqi_low;
		  byte lqi_high;
		  byte probloss;
    }link_info[46];

};





struct	t_entry { 		/* Entry in a topology file (also used	*/
    /*  in messages.			*/
    int32	t_nodeid;	/* ID of a node				*/
    int32	t_status;	/* Status of the node			*/
    byte	t_neighbors[6];	/* The multicast address of neighbors	*/
    byte	t_macaddr[6];	/* The Mac address of a node		*/
    int32       t_bbbid;
};



struct	p_entry {		/* Entry in a ping reply		*/
    int32	pnodeid;	/* ID of a node				*/
    int32	pstatus;	/* Status of the node			*/
};

struct	c_msg {
    int32	clength;	/* The length of the message measured	*/
    /* in octets from the start of c_msg.	*/
    int32	cmsgtyp;	/* Message type as specified above	*/

    union {			/* Items in the union are only needed	*/
        /*  for messages that contain additional*/
        /*  information beyond the message type	*/
        /*  and message length.			*/

        int32	xonoffid;	/* Node ID for XON/XOFF; value	*/
        /*  -1 means "all nodes".	*/

        int32	pingnodeid;	/* PING_REQ */

        struct { 		/* PING_REPLY */
            int32	pingnum; /* Count of entries that follow*/
            struct	p_entry	pingdata[MAXNODES];

        };

        struct { 		/* TOP_REPLY */
            int32	topnum; /* Count of entries that follow	*/
            struct	t_entry	topdata[MAXNODES];
        };

        struct { 		/* NEW_TOP */
            int32	flen;	/* Length of the name in bytes	*/
            byte	fname[File_Name_Size];/* File name to use		*/
        };


	struct {
                  int32 nnodes;
		  char map[MAXNODES][NAMELEN];   /* As a response of C_MAP message */
                 

	};

	struct {
		int32 nbbb;
		int32 bbb_stat[MAX_BBB];
	};

	struct	{
		int32 mon_cmd;
		int32 mon_nodeid;
		bool8 mon_fsrc;
		bool8 mon_fdst;
		byte  mon_ipsrc[16];
		byte  mon_ipdst[16];
	};

        uint32	uptime;		/* TS_RESP (amount of time the	*/
        /* server has been up (in msec).*/


	uint32 ctime;           /* current time */

    };
};

/*---------------
 * UDP port
 * TIME_OUT value
 * ping status
 * ------------*/
#define WS_PORT		55001
#define WS_TIMEOUT	600000
#define WS_NOTACTIV	0
#define WS_ALIVE	1
#define WS_NOTRESP	-1
#define XOFF		0
#define XON		1

extern	int32 nnodes;
extern	int32 nodeid;
extern	int32 online;
extern	int32 *seqnum;
extern	int32 ping_ack_flag[MAXNODES];
extern	byte ack_info[16];
extern	char map_list[MAXNODES][NAMELEN];
extern	struct topo_entry topo[MAXNODES];
extern	struct topo_entry old_topo[MAXNODES];
extern	int32  bbb_stat[MAX_BBB];
extern	byte   bbb_macs[MAX_BBB][6];
