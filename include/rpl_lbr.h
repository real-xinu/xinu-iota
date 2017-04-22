/* rpl_lbr.h - RPL LBR Definitions */

#define	RPLNODE_STATE_FREE	0
#define	RPLNODE_STATE_USED	1

#define	NRPLNODES	50

struct	rplnode {
	byte	state;
	int32	rplindex;
	byte	ipprefix[16];
	int32	ippreflen;
	struct {
	  int32	nindex;
	  byte	pc;
	  byte	ps;
	  int32	texpire;
	} parents[RPL_PARENT_SET_SIZE];
	int32	nparents;
	int32	prefparent;
	uint32	time;
	int32	maxseq;
};

extern	struct rplnode rplnodes[NRPLNODES];
