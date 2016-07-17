/* net.h */

#define NETSTK		8192 		/* Stack size for network setup */
#define NETPRIO		500    		/* Network startup priority 	*/
#define NETBOOTFILE	128		/* Size of the netboot filename	*/

/* Format of a Radio packet carrying IPv6 and UDP */
#define ETH_TYPE_A  0x88b5
#define ETH_TYPE_B  0x88b6

#pragma pack(2)
struct	netpacket	{
	union {
	 struct {
	  byte		net_radtype;	/* Radio Frame control		*/
	  byte		net_radflags;
	  byte		net_raddstpan[2];/* Radio destination PAN	*/
	  byte		net_raddstaddr[8];/* Radio destination EUI64	*/
	  byte		net_radsrcpan[2];/* Radio source PAN		*/
	  byte		net_radsrcaddr[8];/* Radio source EUI64		*/
	 };
	 struct {
	  byte		net_ethpad[8];
	  byte		net_ethdst[6];
	  byte		net_ethsrc[6];
	  uint16	net_ethtype;
	 };
	};
	union {
	 byte		net_radie[1500];/* Radio Information elements	*/
	 struct {
	  byte		net_ipvtch;	/* IP vers, traffic class high	*/
	  byte		net_iptclflh;	/* IP TC low, flow label high	*/
	  uint16	net_ipfll;	/* IP flow label low		*/
	  uint16	net_iplen;	/* IP payload length		*/
	  byte		net_ipnh;	/* IP next header		*/
	  byte		net_iphl;	/* IP hop limit			*/
	  byte		net_ipsrc[16];	/* IP source address		*/
	  byte		net_ipdst[16];	/* IP destination address	*/
	  union {
	   byte		net_ipdata[1500-40];
	   struct {
	    uint16 	net_udpsport;	/* UDP source protocol port	*/
	    uint16	net_udpdport;	/* UDP destination protocol port*/
	    uint16	net_udplen;	/* UDP total length		*/
	    uint16	net_udpcksum;	/* UDP checksum			*/
	    byte	net_udpdata[1500-48];/* UDP payload (1500-above)*/
	   };
	   struct {
	    byte	net_ictype;	/* ICMP message type		*/
	    byte	net_iccode;	/* ICMP code field (0 for ping)	*/
	    uint16	net_iccksum;	/* ICMP message checksum	*/
	    byte	net_icdata[1500-44];/* ICMP payload (1500-above)*/
	   };
	   struct {
	    uint16	net_tcpsport;	/* TCP Source port		*/
	    uint16	net_tcpdport;	/* TCP Destination port		*/
	    int32	net_tcpseq;	/* TCP sequence number		*/
	    int32	net_tcpack;	/* TCP Acknowledgement number	*/
	    uint16	net_tcpcode;	/* TCP data offset and flags	*/
	    uint16	net_tcpwindow;	/* TCP Window size		*/
	    uint16	net_tcpcksum;	/* TCP Checksum			*/
	    uint16	net_tcpurgptr;	/* TCP Urgent Pointer		*/
	    byte	net_tcpdata[1500-40];/* TCP Data		*/
	   };
	  };
	 };
	};
	int32		net_iface;	/* Incoming interface index	*/
};
#pragma pack()

#define	PACKLEN	sizeof(struct netpacket)

struct	ipinfo {
	byte	iptc;	/* IP traffic class	*/
	uint32	ipfl;	/* IP flow label	*/
	byte	iphl;	/* IP hop limit		*/
	byte	ipsrc[16];/* IP source address	*/
	byte	ipdst[16];/* IP dest. address	*/
};

struct network {
	uint32  ipucast;
	uint32  ipbcast;
	uint32  ipmask;
	uint32  ipprefix;
	uint32  iprouter;
	uint32  bootserver;
	bool8   ipvalid;
	byte    ethucast[ETH_ADDR_LEN];
	byte    ethbcast[ETH_ADDR_LEN];
	char    bootfile[NETBOOTFILE];
};

extern struct  network NetData;        /* Local Network Interface      */
extern	bpid32	netbufpool;		/* ID of net packet buffer pool	*/
