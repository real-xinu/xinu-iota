/* net.h */

#pragma pack(1)
struct	a_msg {
	int32	amsgtyp;
    int32	anodeid;
    int32	aseq;
    int32	aacktyp;
    union {
        struct {
            struct {
                byte	amcastaddr[6];
            };
            struct {
                byte lqi_low;
                byte lqi_high;
                byte threshold;
                byte pathloss_ref;
                byte pathloss_exp;
                byte dist_ref;
                byte sigma;
                uint32 distance;
            } link_info[46];
        };
        uint32	ctime;
    };
};
#pragma pack()

/* Structure of an Ethernet packet */
#pragma pack(1)
struct	netpacket_e {
    byte	net_ethdst[ETH_ADDR_LEN];/* Ethernet destination	*/
    byte	net_ethsrc[ETH_ADDR_LEN];/* Ethernet source		*/
    uint16	net_ethtype;		 /* Ethernet type		*/
    union {
        byte	net_ethdata[1500];	 /* Ethernet payload		*/
        struct a_msg msg;
    };
};
#pragma pack()

#define	RAD_FC_FT	0x0007
#define	RAD_FC_FT_BCN	0x0000
#define	RAD_FC_FT_DAT	0x0001
#define	RAD_FC_FT_ACK	0x0002
#define	RAD_FC_FT_MAC	0x0003
#define	RAD_FC_FT_LLDN	0x0004
#define	RAD_FC_FT_MP	0x0005
#define	RAD_FC_SEC	0x0008
#define	RAD_FC_FP	0x0010
#define	RAD_FC_AR	0x0020
#define	RAD_FC_PC	0x0040
#define	RAD_FC_SS	0x0100
#define	RAD_FC_IE	0x0200
#define	RAD_FC_DAM0	0x0000
#define	RAD_FC_DAM1	0x0400
#define	RAD_FC_DAM2	0x0800
#define	RAD_FC_DAM3	0x0C00
#define	RAD_FC_FV0	0x0000
#define	RAD_FC_FV1	0x1000
#define	RAD_FC_FV2	0x2000
#define	RAD_FC_FV3	0x3000
#define	RAD_FC_SAM0	0x0000
#define	RAD_FC_SAM1	0x4000
#define	RAD_FC_SAM2	0x8000
#define	RAD_FC_SAM3	0xC000

/* Structure of a Radio packet */
#pragma pack(1)
struct	netpacket_r {
    byte	net_ethdst[6];
    byte	net_ethsrc[6];
    uint16	net_ethtype;
    byte	net_pad;
    uint16	net_radfc;		/* Frame Control		*/
    byte	net_radseq;		/* Sequence number		*/
    byte	net_raddstpan[2];	/* Destination PAN ID		*/
    byte	net_raddst[8];		/* Destination address		*/
    byte	net_radsrcpan[2];	/* Source PAN ID		*/
    byte	net_radsrc[8];		/* Source address		*/
    byte	net_raddata[1500];	/* Radio payload		*/
};

/* Structure of an IPv6 packet */
#pragma pack(1)
struct	netpacket {
    byte	net_ipvtch;	/* IP version and Traffic class high 	*/
    byte	net_iptclflh;	/* IP Traffic class low, flow label high*/
    uint16	net_ipfll;	/* IP flow label low			*/
    uint16	net_iplen;	/* IP payload length			*/
    byte	net_ipnh;	/* IP next header			*/
    byte	net_iphl;	/* IP hop limit				*/
    byte	net_ipsrc[16];	/* IP source address			*/
    byte	net_ipdst[16];	/* IP destination address		*/
    union {
        byte		net_ipdata[1500-40];/* IP payload		*/
        struct {
            uint16	net_udpsport;	/* UDP source port		*/
            uint16	net_udpdport;	/* UDP destination port		*/
            uint16	net_udplen;	/* UDP header + data length	*/
            uint16	net_udpcksum;	/* UDP checksum			*/
            byte	net_udpdata[1500-48];/* UDP payload		*/
        };
        struct {
            byte	net_ictype;	/* ICMP type			*/
            byte	net_iccode;	/* ICMP code			*/
            uint16	net_iccksum;	/* ICMP checksum		*/
            byte	net_icdata[1500-44];/* ICMP payload		*/
        };
        struct {
            uint16	net_tcpsport;	/* TCP source port		*/
            uint16	net_tcpdport;	/* TCP destination port		*/
            uint32	net_tcpseq;	/* TCP sequence number		*/
            uint32	net_tcpack;	/* TCP acknowledgement number	*/
            uint16	net_tcpcode;	/* Segment type			*/
            uint16	net_tcpwindow;	/* Advertised window size	*/
            uint16	net_tcpcksum;	/* TCP checksum			*/
            uint16	net_tcpurgptr;	/* Urgent pointer		*/
            byte	net_tcpdata[1500-60];/* TCP payload		*/
        };
    };
    int32	net_iface;	/* Network interface index		*/
};
#pragma pack()

#define	PACKLEN	(sizeof(struct netpacket_e)+32)

#define	NETPRIO	500

/* Buffer pool for network packets	*/
extern	bpid32	netbufpool;
