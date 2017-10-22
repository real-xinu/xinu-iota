/* testbed.h */

#define	TBR_TYPE_ASSIGN		1
#define	TBR_TYPE_NPING		2
#define	TBR_TYPE_NPING_ALL	3
#define	TBR_TYPE_NPINGALL	4

struct	tbreq {
	uint32	type;
	byte	dst[6];
	bool8	waiting;
	pid32	waitpid;
	uint32	nodeid;
	//union {
	  int32	topoidx;
	  struct {
	    byte	mcast[6];
	    struct {
	      byte	lqi_low;
	      byte	lqi_high;
	      byte	probloss;
	    } link_info[46];
	  };
	//};
};

#define	TB_STATE_WAIT	1

#define	TB_QSIZE	80

struct	tbinfo {
	uint32	state;
	uint32	nextid;
	struct	tbreq reqq[TB_QSIZE];
	int32	thead;
	int32	ttail;
	int32	tcount;
	sid32	tsem;
	uint32	seq;
	byte	waitaddr[6];
	pid32	pid;
	bool8	monitor;
	byte	mon_ethaddr[6];
};

extern	struct tbinfo testbed;

#define	TMAP_STATE_USED	1
#define	TMAP_STATE_FREE	0

struct	tmapentry {
	int32	state;
	uint32	nodeid;
	uint32	bbbid;
};

extern	struct tmapentry tbedmap[MAX_BBB];
