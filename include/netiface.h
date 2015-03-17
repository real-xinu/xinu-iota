/* netiface.h */

/* Format of Interface IP address */

struct	ifipaddr {
	byte	ipaddr[16];
	uint16	ippreflen;
};

/* Interface types */

#define IF_TYPE_RADIO	1
#define IF_TYPE_ETH	2

/* States of interface */

#define IF_UP		1
#define IF_DOWN		0

/* Maximum number of IP addressesper interface */

#define IF_NIPUCAST	5
#define IF_NIPMCAST	5

#define IF_HALEN	8	/* MAX HW address length	*/
#define NIFACES		2	/* Number of interfaces		*/

/* Index in the interface table */

#define IF_RADIO	0
#define IF_ETH		1

/* Format of Network Interface */

struct	ifentry {
	byte	if_state;	/* State of the interface		*/
	byte	if_type;	/* Type of interface			*/
	did32	if_dev;		/* Index in the device switch table	*/

	byte	if_hwucast[IF_HALEN];	/* HW unicast address		*/
	byte	if_hwbcast[IF_HALEN];	/* HW broadcast address		*/
	int32	if_halen;		/* HW Address length		*/

	struct	ifipaddr if_ipucast[IF_NIPUCAST];/* IP ucast addresses	*/
	byte	if_nipucast;	/* No. of IP unicast addresses		*/
	struct	ifipaddr if_ipmcast[IF_NIPMCAST];/* IP mcast addresses	*/
	byte	if_nipmcast;	/* No. of IP multicast addresses	*/
	struct	nd_nce	if_ncache[ND_NC_SLOTS];
	struct	nd_info	if_ndData;/* Neighbor Discovery information	*/

	/* For emulation only */
	struct	netpacket *pktbuf[10];
	int32	head;
	int32	tail;
	int32	count;
	sid32	isem;
};

extern	struct ifentry if_tab[NIFACES];
