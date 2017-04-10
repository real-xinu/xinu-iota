/* nd.h - Neighbor Discovery related definitions */

/* Structure of a router advertisement message */
#pragma pack(1)
struct	nd_rtradv {
	byte	nd_curhl;
	byte	nd_m:1;
	byte	nd_o:1;
	byte	nd_rtrlife;
	uint32	nd_reachtime;
	uint32	nd_retranstime;
	byte	nd_opts[];
};
#pragma pack(0)

/* Structure of a neighbor solicitation message */
#pragma pack(1)
struct nd_nbrsol {
	uint32	nd_resvd;
	byte	nd_tgtaddr[16];
	byte	nd_opts[];
};
#pragma pack(0)

#define	ND_NA_R	0x80
#define	ND_NA_S	0x40
#define	ND_NA_O	0x20

/* Structure of a neighbor advertisement message */
#pragma pack(1)
struct	nd_nbradv {
	byte	nd_rso;
	byte	nd_res[3];
	byte	nd_tgtaddr[16];
	byte	nd_opts[];
};
#pragma pack(0)

#define	NDOPT_TYPE_SLLA	1
#define	NDOPT_TYPE_TLLA	2
#define	NDOPT_TYPE_AR	33

/* Structure of a neighbor discovery option */
#pragma pack(1)
struct	nd_opt {
	byte	ndopt_type;
	byte	ndopt_len;
	union { /* Source/target link-layer address */
	  struct {
	    byte	ndopt_addr[16];
	  };
	  struct {
	    byte	ndopt_preflen;
	    byte	ndopt_l:1;
	    byte	ndopt_a:1;
	    uint32	ndopt_validlife;
	    uint32	ndopt_preferlife;
	    uint32	ndopt_resvd;
	    byte	ndopt_prefix[16];
	  };
	  struct {
	    byte	ndopt_status;
	    byte	ndopt_res[3];
	    uint16	ndopt_reglife;
	    byte	ndopt_eui64[8];
	  };
	};
};
#pragma pack()

#define	NC_STATE_FREE	0
#define	NC_STATE_USED	1

#define	NC_TYPE_GC	0
#define	NC_TYPE_TEN	1
#define	NC_TYPE_REG1	2
#define	NC_TYPE_REG2	3

#define	NC_RSTATE_INC	0
#define	NC_RSTATE_RCH	1
#define	NC_RSTATE_STL	2
#define	NC_RSTATE_DLY	3
#define	NC_RSTATE_PRB	4

#define	ND_REACHABLE_TIME		30000
#define	ND_RETRANS_TIMER		1000
#define	ND_DELAY_FIRST_PROBE_TIME	5000
#define	ND_MAX_UNICAST_SOLICIT		3
#define	ND_MAX_MULTICAST_SOLICIT	3

#define	ND_NCACHE_SIZE	10

#define NC_PKTQ_SIZE	2

/* Structure of a neighbr cache entry */
struct	nd_ncentry {
	int32	nc_state;
	int32	nc_type;
	int32	nc_iface;
	int32	nc_rstate;
	byte	nc_ipaddr[16];
	byte	nc_hwaddr[IF_MAX_HALEN];
	int32	nc_isrouter;
	int32	nc_texpire;
	int32	nc_retries;
	void	*nc_pktq[NC_PKTQ_SIZE];
	int32	nc_pqhead;
	int32	nc_pqtail;
	int32	nc_pqcount;
};

extern	struct	nd_ncentry nd_ncache[];
