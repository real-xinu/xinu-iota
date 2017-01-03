/* ip.h  -  IP related definitions */

#define	IP_IP		41
#define	IP_ICMP		58
#define	IP_TCP		6
#define	IP_UDP		17

#define	IP_EXT_HBH	0
#define	IP_EXT_RH	43
#define	IP_EXT_RH_RT0	0

/* Structure of IP Extension Header */
struct	ip_ext_hdr {
	byte	ipext_nh;	/* Next header		*/
	byte	ipext_len;	/* Header length	*/
	union {
	  struct { /* Routing Header */
	    byte	ipext_rhrt;	/* Routing Type		*/
	    byte 	ipext_rhsegs;	/* Segments left	*/
	    uint32	ipext_rhresvd;	/* Reserved		*/
	    byte	ipext_rhaddrs[][16];
	    			 	/* List of addresses	*/
	  };
	  struct { /* RPL Hop-by-Hop */
	    byte	ipext_optype;	/* Option type		*/
	    byte	ipext_optlen;	/* Option length	*/
	    byte	ipext_rsvd:5;	/* Reserved		*/
	    byte	ipext_f:1;
	    byte	ipext_r:1;
	    byte	ipext_o:1;
	    byte	ipext_rplinsid; /* RPL instance ID	*/
	    uint16	ipext_sndrank;	/* Sender's rank	*/
	  };
	};
};

extern	byte	ip_llprefix[];
extern	byte	ip_unspec[];

#define	IPFW_STATE_FREE	0
#define	IPFW_STATE_USED	1

#define	IPFW_TABSIZE	10

/* Structure of IP forwarding table entry */
struct	ip_fwentry {
	int32	ipfw_state;
	int32	ipfw_iface;
	struct	ifipaddr ipfw_prefix;
	int32	ipfw_onlink;
	byte	ipfw_nxthop[16];
};

struct ipinfo{
        byte    ipsrc[16];
	byte    ipdst[16];
	byte    iphl;
	uint16  port;
};


extern	struct	ip_fwentry ip_fwtab[];

extern	byte	ip_nd_snmcprefix[];
extern	byte	ip_allnodesmc[];

#define	isipmc(x)	((*(x)) == 0xff)
#define	isipllu(x)	(!memcmp((x), ip_llprefix, 8))
#define	isipunspec(x)	(!memcmp((x), ip_unspec, 16))
