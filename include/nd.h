/* nd.h - Neighbor Discovery definitions */

#define ICMP_TYPE_RTRSOL	133	/* ICMP type for router solicitation	*/
#define ICMP_TYPE_RTRADV	134	/* ICMP type for router advertisement	*/
#define ICMP_TYPE_NBRSOL	135	/* ICMP type for neighbor solicitation	*/
#define ICMP_TYPE_NBRADV	136	/* ICMP type for neighbor advertisement	*/

#define ND_TYPE_SLLAO		1	/* Source Link layer address option	*/
#define ND_TYPE_TLLAO		2	/* Target link layer address option	*/
#define ND_TYPE_PIO		3	/* Prefix information option		*/
#define ND_TYPE_ARO		33	/* Address registration option		*/

#define ND_NC_SLOTS		10	/* Number of Neighbor Cache slots	*/

#define ND_NCE_FREE		0	/* NC Entry free	*/
#define ND_NCE_INCOM		1	/* NC Entry incomplete	*/
#define ND_NCE_REACH		2	/* NC entry reachable	*/
#define ND_NCE_STALE		3	/* NC entry stale	*/
#define ND_NCE_DELAY		4	/* NC entry delay	*/
#define ND_NCE_PROBE		5	/* NC entry probe	*/

#define ND_NCE_GRB		1	/* NC entry grabage-collectible	*/
#define ND_NCE_REG		2	/* NC entry registered		*/
#define ND_NCE_TEN		3	/* NC entry tentative		*/

#define ND_PROCSSIZE		8192
#define ND_PROCPRIO		400

#pragma pack(1)

/* Format of a Router Solicitation message */

struct	nd_rtrsol {
	uint32	res;	/* Reserved	*/
	byte	opts[];
};

/* Format of Router Advertisement message */

struct	nd_rtradv {
	byte	currhl;	/* Current Hop limit	*/
	byte	res:6;	/* Reserved		*/
	byte	o:1;	/* Other flag		*/
	byte	m:1;	/* Managed flag		*/
	uint16	rtrlife;/* Router lifetime	*/
	uint32	reachable;/* Reachable time	*/
	uint32	retrans;/* Retrans timer	*/
	byte	opts[];
};

/* Format of Neighbor Solicitation message */

struct nd_nbrsol {
	uint32	res;	/* Reserved		*/
	byte	tgtaddr[16];/* Target address	*/
	byte	opts[];
};

/* Format of Neighbor Advertisement message */

struct	nd_nbradv {
	byte	res1:5;	/* Reserved		*/
	byte	o:1;	/* Override flag	*/
	byte	s:1;	/* Solicited flag	*/
	byte	r:1;	/* Router flag		*/
	byte	res2[3];/* Reserved		*/
	byte	tgtaddr[16];/* Target address	*/
	byte	opts[];
};

/* Format of Link Layer Address option */

struct	nd_llao {
	byte	type;	/* Option type		*/
	byte	len;	/* Option length	*/
	byte	lladdr[14];/* Link Layer Address*/
};

/* Format of Prefix Information option */

struct	nd_pio {
	byte	type;	/* Option type		*/
	byte	len;	/* Option length	*/
	byte	preflen;/* Prefix length	*/
	byte	res1:6;	/* Reserved		*/
	byte	a:1;	/* Autoconfiguration	*/
	byte	l:1;	/* On-link flag		*/
	uint32	validlife;/* Valid lifetime	*/
	uint32	preflife;/* Preferred lifetime	*/
	uint32	res2;
	byte	prefix[16];
};

/* Format of Address Registration option */

struct	nd_aro {
	byte	type;	/* Option type		*/
	byte	len;	/* Option length	*/
	byte	status;	/* Registration status	*/
	byte	res[3];	/* Reserved		*/
	uint16	reglife;/* Registration lifetime*/
	byte	eui64[8];/* EUI 64 Address	*/
};

#pragma pack()

/* Format of a Neighbor Cache entry */

struct	nd_nce {
	byte	state;	/* NCE State		*/
	byte	type;	/* NCE Type		*/
	pid32	pid;	/* PID of waiting proc	*/
	sid32	waitsem;/* Semaphore of entry	*/
	byte	ipaddr[16];/* NCE IP address	*/
	byte	hwucast[8];
			/* NCE EUI64 address	*/
	uint32	reglife;/* NCE lifetime		*/
	uint32	ttl;	/* NCE time to live	*/
};

struct	nd_info {
	bool8	isrouter;
	bool8	sendadv;
	bool8	managed;
	bool8	other;
	uint32	reachable;
	uint32	retrans;
	byte	currhl;
	uint32	lifetime;
};
