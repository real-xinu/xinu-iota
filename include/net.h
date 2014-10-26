/* net.h */

#define NETSTK		8192 		/* Stack size for network setup */
#define NETPRIO		500    		/* Network startup priority 	*/
#define NETBOOTFILE	128		/* Size of the netboot filename	*/

/* Constants used in the networking code */

#define	ETH_ARP     0x0806		/* Ethernet type for ARP	*/
#define	ETH_IP      0x0800		/* Ethernet type for IP		*/
#define	ETH_IPv6    0x86DD		/* Ethernet type for IPv6	*/

/* Format of an Ethernet packet carrying IPv4 and UDP */

#pragma pack(2)
struct	netpacket	{
	uint16		net_panfc;	/* 802.15.4 Frame Control	*/
	uint16		net_pandstpan;	/* 802.15.4 Destination PAN ID	*/
	byte		net_pandst[8];	/* 802.15.4 Destination EUI64	*/
	uint16		net_pansrcpan;	/* 802.15.4 Source PAN ID	*/
	byte		net_pansrc[8];	/* 802.15.4 Source EUI64	*/
	byte		net_ipvtch;	/* IPv6 version, traf class high*/
	byte		net_iptclflh;	/* IPv6 traf class low, flow	*/
					/* label high			*/
	uint16		net_ipfll;	/* IPv6 Flow label low		*/
	uint16		net_iplen;	/* IPv6 payload length		*/
	byte		net_ipnh;	/* IPv6 Next header		*/
	byte		net_iphl;	/* IPv6 Hop Limit		*/
	byte		net_ipsrc[16];	/* IPv6 source address		*/
	byte		net_ipdst[16];	/* IPv6 destination address	*/
	union {
	 struct {
	  uint16 	net_udpsport;	/* UDP source protocol port	*/
	  uint16	net_udpdport;	/* UDP destination protocol port*/
	  uint16	net_udplen;	/* UDP total length		*/
	  uint16	net_udpcksum;	/* UDP checksum			*/
	  byte		net_udpdata[1576-28];/* UDP payload (1500-above)*/
	 };
	 struct {
	  byte		net_ictype;	/* ICMP message type		*/
	  byte		net_iccode;	/* ICMP code field (0 for ping)	*/
	  uint16	net_iccksum;	/* ICMP message checksum	*/
	  byte		net_icdata[1576-28];/* ICMP payload (1500-above)*/
	 };
	};
	uint16		net_iface;
};
#pragma pack()

#define	PACKLEN	sizeof(struct netpacket)

extern	bpid32	netbufpool;		/* ID of net packet buffer pool	*/

struct	network	{
	uint32	ipucast;
	uint32	ipbcast;
	uint32	ipmask;
	uint32	ipprefix;
	uint32	iprouter;
	uint32	bootserver;
	bool8	ipvalid;
	bool8	coord;
	byte	ethucast[ETH_ADDR_LEN];
	byte	ethbcast[ETH_ADDR_LEN];
	char	bootfile[NETBOOTFILE];
};

extern	struct	network NetData;	/* Local Network Interface	*/
