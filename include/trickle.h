/* trickle.h */

#define	TRK_STATE_FREE	0
#define	TRK_STATE_USED	1
#define	TRK_STATE_1	2
#define	TRK_STATE_2	3

struct _trickle {
	int32	state;
	pid32	tproc;
	int32	Imin;
	int32	Imax;
	int32	k;
	int32	c;
	bool8	reset;
};

#define NTRICKLE	1

extern	struct	_trickle trickle[NTRICKLE];
