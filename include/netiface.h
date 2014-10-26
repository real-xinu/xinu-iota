/* netiface.h */

#define	EUI64_ADDR_LEN	8	/* Length in bytes of EUI64 address	*/

#define	IF_NIPUCAST	5	/* No. of IPv6 unicast addresses	*/
#define IF_NIPMCAST	5	/* No. of IPv6 multicast addresses	*/

/* Interface IPv6 address structure */

struct	ifipaddr {
	byte	ipaddr[16];	/* IPv6 address	*/
	uint32	preflen;		/* Prefix length*/
};

#define	IF_DOWN	0	/* Interface is down	*/
#define	IF_UP	1	/* Interface is up	*/

#define NIFACES	1	/* No. of interfaces	*/

/* Network Interface structure */

struct	ifentry {
	byte	if_state;	/* Interface State - UP / Down		*/

	byte	if_eui64[EUI64_ADDR_LEN]; /* Unique EUI64 address	*/

	struct	ifipaddr if_ipucast[IF_NIPUCAST]; /* IPv6 Ucast addrs	*/
	struct	ifipaddr if_ipmcast[IF_NIPMCAST]; /* IPv6 Mcast addrs	*/
	byte	if_nipucast;	/* No. of valid IP ucast addresses	*/
	byte	if_nipmcast;	/* No. of valid IP mcast addresses	*/

	byte	if_iprouter;	/* Default IP router for interface	*/

	uint32	if_mtu;		/* MTU for this interface		*/

	uint32	if_dev;		/* Device ID of Radio			*/
};

extern	struct ifentry if_tab[NIFACES];
