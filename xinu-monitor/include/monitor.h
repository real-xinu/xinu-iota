/* monitor.h */

#define	WIRESHARK_IP	"128.10.137.51"
#define	WIRESHARK_PORT	43010

#define	MCMD_START	1
#define	MCMD_STOP	2

struct	monitor_msg {
	uint32	pktlen;		/* Length of packet	*/
	byte	pktdata[];	/* Packet data		*/
};

#define	MFILTER_INACTIVE	0
#define	MFILTER_ACTIVE		1

struct	_filter {
	int32	state;
	int32	udpslot;
	int32	nodeid;
	bool8	fsrc;
	bool8	fdst;
	byte	ipsrc[16];
	byte	ipdst[16];
};

extern	struct _filter mfilter;
