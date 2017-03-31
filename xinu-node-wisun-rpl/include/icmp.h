/* icmp.h - ICMP related definitions */

#define	ICMP_TYPE_ERQ	128
#define	ICMP_TYPE_ERP	129
#define	ICMP_TYPE_NS	135
#define	ICMP_TYPE_NA	136
#define	ICMP_TYPE_RPL	155

#define	ICMP_CODE_RPL_DIS	0
#define	ICMP_CODE_RPL_DIO	1
#define	ICMP_CODE_RPL_DAO	2
#define	ICMP_CODE_RPL_DAOK	3
#define	ICMP_CODE_RPL_CC	0x8A

#pragma pack(1)
struct	icmp_hdr {
	byte	icmp_type;
	byte	icmp_code;
	byte	icmp_data[];
};
#pragma pack()

#define	ICENTRY_FREE	0
#define	ICENTRY_USED	1
#define	ICENTRY_RECV	2

#define	ICENTRY_PQSIZE	3

/* Structure of ICMP registration table entry */
struct	icentry {
	int32	icstate;/* State of the entry		*/
	byte	ictype;	/* ICMP type			*/
	byte	iccode;	/* ICMP code			*/
	byte	icremip[16];/* Remote IP address	*/
	byte	iclocip[16];/* Local IP address		*/
	int32	iciface;/* Interface index		*/
	struct	netpacket *icpktq[ICENTRY_PQSIZE];
			/* Waiting packets		*/
	int32	ichead;	/* Head of packet queue		*/
	int32	ictail;	/* Tail of packet queue		*/
	int32	iccount;/* No. of packets in queue	*/
	pid32	icpid;	/* PID of process waiting	*/
};

#define	ICMP_TABSIZE	10
extern	struct	icentry icmptab[];
