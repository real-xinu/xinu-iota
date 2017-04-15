/* monitor.h */

#define	MGT_SRVPORT	43009
#define	MGT_CLTPORT	43008

#define	WIRESHARK_IP	"128.10.137.51"
#define	WIRESHARK_PORT	43010

#pragma pack(1)
struct	monitor_msg {
	uint32	pktlen;
	byte	pktdata[];
};
#pragma pack()

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

#define	MCMD_START	1
#define	MCMD_STOP	2

#pragma pack(1)
struct	monitor_cmd {
	uint32	cmd;
	uint32	nodeid;
	bool8	fsrc;
	bool8	fdst;
	byte	ipsrc[16];
	byte	ipdst[16];
};
#pragma pack()
