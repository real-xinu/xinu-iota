/* netiface.h */

/* Network interface states */
#define IF_DOWN	0
#define	IF_UP	1

/* Network interface types */
#define	IF_TYPE_ETH	0
#define	IF_TYPE_RAD	1

/* Hardware address lengths */
#define	IF_HALEN_ETH	6
#define	IF_HALEN_RAD	8

/* Limits for an interface */
#define	IF_MAX_HALEN	8
#define	IF_MAX_NIPUCAST	5
#define	IF_MAX_NIPMCAST	5
#define	IF_MAX_NIPPREF	5

#define IFQSIZ          10

/* Structure of an IPv6 address */
struct	ifipaddr {
	byte	ipaddr[16];
	uint32	prefixlen;
};

/* No. of network interfaces */
#define	NIFACES	2

/* Structure of a Network Interface */
struct	netiface {

	/* State of the network interface */
	int32	if_state;

	/* Type of the network interface */
	int32	if_type;

	/* Hardware address length and addresses */
	int32	if_halen;
	byte	if_hwucast[IF_MAX_HALEN];
	byte	if_hwbcast[IF_MAX_HALEN];

	/* IPv6 addresses associated with this interface */
	int32	if_nipucast;
	int32	if_nipmcast;
	struct	ifipaddr if_ipucast[IF_MAX_NIPUCAST];
	struct	ifipaddr if_ipmcast[IF_MAX_NIPMCAST];

	/* Sequence number for radio interface */
	byte	if_seq;

	/* Neighbor Discovery related fields */
	int32	if_nd_reachtime;
	int32	if_nd_retranstime;

	/* Device entry in the device table */
	did32	if_dev;

	/* Input queue for this interface */
	void	*if_inputq[IFQSIZ];
	int32	if_iqhead;
	int32	if_iqtail;
	sid32	if_iqsem;
};

/* Table of network interfaces */
extern	struct	netiface iftab[];
