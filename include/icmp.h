/* icmp.h - definintions for the Internet Control Message Protocol */

#define	ICMP_SLOTS	10		/* num. of open ICMP endpoints	*/
#define	ICMP_QSIZ	8		/* incoming packets per slot	*/

/* Constants for the state of an entry */

#define	ICMP_FREE	0		/* entry is unused		*/
#define	ICMP_USED	1		/* entry is being used		*/
#define	ICMP_RECV	2		/* entry has a process waiting	*/

#define ICMP_HDR_LEN	4		/* bytes in an ICMP header	*/

/* ICMP message types for ping */

#define	ICMP_ECHOREPLY	129		/* ICMP Echo Reply message	*/
#define ICMP_ECHOREQST	128		/* ICMP Echo Request message	*/

/* Format of ICMPv6 echo packets */

struct	icmp_echo {
	uint16	id;	/* Identifier	*/
	uint16	seq;	/* Sequence no.	*/
};

/* table of processes that are waiting for ping replies */

struct	icmpentry {			/* entry in the ICMP table	*/
	int32	icstate;		/* state of entry: free/used	*/
	int32	iciface;		/* interface index		*/
	byte	iclocip[16];		/* local IP address		*/
	byte	icremip[16];		/* remote IP address		*/
	byte	ictype;			/* ICMP type			*/
	byte	iccode;			/* ICMP code			*/
	int32	ichead;			/* index of next packet to read	*/
	int32	ictail;			/* index of next slot to insert	*/
	int32	iccount;		/* count of packets enqueued	*/
	pid32	icpid;			/* ID of waiting process	*/
	struct	netpacket *icqueue[ICMP_QSIZ];/* circular packet queue	*/
};

extern	struct	icmpentry icmptab[];	/* table of UDP endpoints	*/
