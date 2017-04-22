/* RPL related definitions */

/* Structure of Destination Information Solicitation message */
#pragma pack(1)
struct	rpl_dis {
	byte	rpl_flags;	/* Flags		*/
	byte	rpl_rsvd;	/* Reserved		*/
	byte	rpl_opts[];	/* RPL options		*/
};
#pragma pack()

#define	RPL_DIO_MOP0	0
#define	RPL_DIO_MOP1	1
#define	RPL_DIO_MOP2	2
#define	RPL_DIO_MOP3	3

/* Structure of Destination Information Object */
#pragma pack(1)
struct	rpl_dio {
	byte	rpl_insid;	/* RPL instance ID	*/
	byte	rpl_vers;	/* Version number	*/
	uint16	rpl_rank;	/* Rank			*/
	byte	rpl_prf:3;	/* DODAG preference	*/
	byte	rpl_mop:3;	/* Mode of Operation	*/
	byte	rpl_z:1;	/* Zero bit		*/
	byte	rpl_g:1;	/* Grounded bit		*/
	byte	rpl_dtsn;	/* Dst. Adv. Trig. Seq.	*/
	byte	rpl_flags;	/* Flags		*/
	byte	rpl_rsvd;	/* Reserved		*/
	byte	rpl_dodagid[16];/* DODAG ID		*/
	byte	rpl_opts[];	/* RPL options		*/
};
#pragma pack()

/* Structure of Destination Advertisement Object message */
#pragma pack(1)
struct	rpl_dao {
	byte	rpl_insid;	/* RPL instance ID	*/
	byte	rpl_flags:6;	/* RPL flags		*/
	byte	rpl_d:1;	/* DODAG ID present?	*/
	byte	rpl_k:1;	/* Acknowledgement req.	*/
	byte	rpl_rsvd;	/* Reserved		*/
	byte	rpl_daoseq;	/* DAO sequence number	*/
	byte	rpl_dodagid[16];/* DODAG ID		*/
	byte	rpl_opts[];
};
#pragma pack()

/* Structure of DAO Acknowledgement message */
#pragma pack(1)
struct	rpl_daoack {
	byte	rpl_insid;	/* RPL instance ID	*/
	byte	rpl_rsvd:7;	/* Reserved		*/
	byte	rpl_d:1;	/* DODAG ID present?	*/
	byte	rpl_daoseq;	/* DAO sequence number	*/
	byte	rpl_status;	/* Status		*/
	byte	rpl_dodagid[16];/* DODAG ID		*/
	byte	rpl_opts[];
};
#pragma pack()

/* Structure of Consistency Check message */
#pragma pack(1)
struct	rpl_cc {
	byte	rpl_insid;	/* RPL instance ID	*/
	byte	rpl_flags:7;	/* Flags		*/
	byte	rpl_r:1;	/* Is this a response?	*/
	uint16	rpl_nonce;	/* Nonce value		*/
	byte	rpl_dodagid[16];/* DODAG ID		*/
	uint32	rpl_dstctr;	/* Destination counter	*/
	byte	rpl_opts[];
};
#pragma pack()

#define	RPLOPT_TYPE_P1		0
#define	RPLOPT_TYPE_PN		1
#define	RPLOPT_TYPE_DM		2
#define	RPLOPT_TYPE_RI		3
#define	RPLOPT_TYPE_DC		4
#define	RPLOPT_TYPE_TG		5
#define	RPLOPT_TYPE_TI		6
#define	RPLOPT_TYPE_SI		7
#define	RPLOPT_TYPE_PI		8
#define	RPLOPT_TYPE_TD		9

/* Structure of PAD1 RPL option */
#pragma pack(1)
struct	rplopt_pad1 {
	byte	rplopt_type;	/* RPL option type		*/
};
#pragma pack()

/* Structure of PADN RPL option	*/
#pragma pack(1)
struct rplopt_padn {
	byte	rplopt_type;	/* RPL option type	*/
	byte	rplopt_len;	/* RPL option length	*/
	byte	rplopt_padding[];/* Option padding	*/
};
#pragma pack()

/* Structure of DAG Metric container option */
#pragma pack()
struct	rplopt_dagmetric {
	byte	rplopt_type;	/* RPL option type	*/
	byte	rplopt_len;	/* RPl option length	*/
	byte	rplopt_metric[];/* Metric data		*/
};
#pragma pack()

/* Structure of Route Information option */
#pragma pack(1)
struct	rplopt_ri {
	byte	rplopt_type;	/* RPL option type	*/
	byte	rplopt_len;	/* RPL option length	*/
	byte	rplopt_preflen;	/* Prefix length	*/
	byte	rplopt_rsvd1:3;	/* Reserved bits	*/
	byte	rplopt_prf:2;	/* Route preference	*/
	byte	rplopt_rsvd2:3;	/* Reserved bits	*/
	uint32	rplopt_rtlife;	/* Route lifetime	*/
	byte	rplopt_prefix[16];/* Prefix		*/
};
#pragma pack()

/* Structure of DODAG Configuration option */
#pragma pack(1)
struct	rplopt_dodagconfig {
	byte	rplopt_type;	/* RPL option type	*/
	byte	rplopt_len;	/* RPL option length	*/
	byte	rplopt_pcs:3;	/* Path Control Sequence*/
	byte	rplopt_a:1;	/* Auth. enabled?	*/
	byte	rplopt_flags:4;	/* Flags		*/
	byte	rplopt_diointdbl;/* DIO inter. doublings*/
	byte	rplopt_diointmin;/* DIo interval min.	*/
	byte	rplopt_dioredun;/* Redundancy constant	*/
	uint16	rplopt_maxrankinc;/* Max. rank increase	*/
	uint16	rplopt_minhoprankinc;
				/* Min. hop rank incr.	*/
	uint16	rplopt_ocp;	/* Objective Code Point	*/
	byte	rplopt_rsvd;	/* Reserved		*/
	byte	rplopt_deflife;	/* Default lifetime	*/
	uint16	rplopt_lifeunit;/* Lifetime unit (secs)	*/
};
#pragma pack()

/* Structure of RPl target option */
#pragma pack(1)
struct	rplopt_tgt {
	byte	rplopt_type;	/* RPl option type	*/
	byte	rplopt_len;	/* RPL option length	*/
	byte	rplopt_flags;	/* Flags		*/
	byte	rplopt_preflen;	/* Prefix length	*/
	byte	rplopt_tgtprefix[16];/* Target prefix	*/
};
#pragma pack()

/* Structure of Transit Information option */
#pragma pack(1)
struct	rplopt_transitinfo {
	byte	rplopt_type;	/* RPL option type	*/
	byte	rplopt_len;	/* RPL option length	*/
	byte	rplopt_flags:7;	/* Flags		*/
	byte	rplopt_e:1;	/* External		*/
	byte	rplopt_pc;	/* Path Control		*/
	byte	rplopt_pathseq;	/* Path sequence	*/
	byte	rplopt_pathlife;/* Path lifetime	*/
	byte	rplopt_parent[16];/* Parent address	*/
};
#pragma pack()

/* Structure of Solicitate Information option */
struct	rplopt_solinfo {
	byte	rplopt_type;	/* RPL option type	*/
	byte	rplopt_len;	/* RPL option length	*/
	byte	rplopt_insid;	/* RPL instance ID	*/
	byte	rplopt_flags:5;	/* Flags		*/
	byte	rplopt_d:1;	/* DODAGID predicate	*/
	byte	rplopt_i:1;	/* Instance Id predicate*/
	byte	rplopt_v:1;	/* Version predicate	*/
	byte	rplopt_dodagid[16];/* DODAG ID		*/
	byte	rplopt_vers;	/* Version number	*/
};
#pragma pack()

/* Structure of Prefix Information option */
#pragma pack(1)
struct	rplopt_pi {
	byte	rplopt_type;	/* RPL option type	*/
	byte	rplopt_len;	/* RPL option length	*/
	byte	rplopt_preflen;	/* Prefix length	*/
	byte	rplopt_rsvd1:5;	/* Reserved		*/
	byte	rplopt_r:1;	/* Router?		*/
	byte	rplopt_a:1;	/* SLAAC?		*/
	byte	rplopt_l:1;	/* On-link?		*/
	uint32	rplopt_validlife;/* Valid lifetime	*/
	uint32	rplopt_preflife;/* Preferred lifetime	*/
	uint32	rplopt_rsvd2;	/* Reserved		*/
	byte	rplopt_prefix[16];/* Prefix		*/
};
#pragma pack()

/* Structure of Target Descriptor option */
#pragma pack(1)
struct	rplopt_tgtdesc {
	byte	rplopt_type;	/* RPl option type	*/
	byte	rplopt_len;	/* RPl option length	*/
	byte	rplopt_desc[];	/* Descriptor		*/
};
#pragma pack()

#define	RPL_MAX_PARENTS		5
#define	RPL_MAX_DODAGS		5

/* Structure of RPL parent */
struct	rpl_neighbor {
	byte	lluip[16];	/* Link-local IP address	*/
	byte	gip[16];	/* Global IP address		*/
	uint16	rank;		/* Rank of neighbor		*/
	uint16	pathcost;	/* Path cost			*/
	uint16	pathrank;	/* Path rank			*/
	bool8	parent;		/* Is this neighbor a parent?	*/
};

struct	rpl_route {
	byte	prefix[16];
	int32	preflen;
	uint32	lifetime;
};

struct	rpl_prefix {
	byte	prefix[16];
	int32	prefixlen;
	uint32	validlife;
	uint32	preflife;
	uint32	texpire;
};

#define	RPL_STATE_FREE	0
#define	RPL_STATE_USED	1

#define	RPL_OCP_MRHOF	1

#define	MAX_RPL_NEIGHBORS	5
#define	MAX_RPL_PARENTS		2
#define	MAX_RPL_ROUTES		5
#define	MAX_RPL_PREFIX		5

#define	RPL_MAX_PATH_COST		32768
#define	RPL_PARENT_SET_SIZE		2
#define	RPL_PARENT_SWITCH_THRESHOLD 	192

/* Structure of RPL table entry */
struct	rplentry {
	int32	state;		/* State Free/Used		*/
	int32	iface;		/* Interface index		*/
	bool8	root;		/* Are we root?			*/
	byte	rplinsid;	/* RPL instance ID		*/
	byte	dodagid[16];	/* DODAG ID			*/
	byte	vers;		/* DODAG version		*/
	bool8	grounded;	/* Grounded?			*/
	byte	prf;		/* DODAG preference		*/
	byte	mop;		/* Mode of operation		*/
	byte	dtsn;		/* Dst. Adv. Trig. Seq.		*/
	byte	pcs;		/* Path Control Sequence	*/
	byte	pcmask;		/* Path Control Mask		*/
	byte	diointdbl;	/* Interval doublings		*/
	byte	diointmin;	/* Interval min.		*/
	byte	dioredun;	/* Redundancy constant		*/
	uint16	maxrankinc;	/* Max. rank increase		*/
	uint16	minhoprankinc;	/* Min. hop rank increas	*/
	uint16	ocp;		/* Objective Code Point		*/
	byte	deflife;	/* Default lifetime		*/
	uint16	lifeunit;	/* Lifetime unit (secs)		*/
	uint16	rank;		/* Our rank in DODAG		*/
	int32	nneighbors;	/* No. of RPL neighbors		*/
	struct	rpl_neighbor neighbors[MAX_RPL_NEIGHBORS];
				/* List of RPL neighbors	*/
	int32	nparents;	/* No. of parents		*/
	int32	prefparent;	/* Preferred parent idx		*/
	int32	parents[RPL_MAX_PARENTS];
				/* List of RPl parents		*/
	int32	nroutes;	/* No. of routes		*/
	struct	rpl_route routes[MAX_RPL_ROUTES];
				/* List of RPL routes		*/
	int32	nprefixes;	/* No. of prefixes		*/
	struct	rpl_prefix prefixes[MAX_RPL_PREFIX];
				/* List of RPL prefixes		*/

	bool8	daok;
	bool8	daokwait;
	uint32	daoktime;
	byte	daoseq;

	/* MRHOF related fields */
	uint32	cur_min_path_cost;

	pid32	timerproc;

	int32	trickle;
};

#define	NRPL	1

#define	RPL_DAGRank(x, r)	((r)/((x)->minhoprankinc))

extern	struct	rplentry rpltab[NRPL];

extern	byte	ip_allrplnodesmc[];
