/* netiface.h */

/* Format of Interface IP address */

struct	ifipaddr {
	byte	ipaddr[16];
	uint16	ippreflen;
};

/* Interface types */

#define IF_TYPE_RADIO	1

/* States of interface */

#define IF_UP	1
#define IF_DOWN	0

#define IF_NIPUCAST	5
#define IF_NIPMCAST	5

#define NIFACES	1

/* Format of Network Interface */

struct	ifentry {
	byte	if_state;	/* State of the interface	*/
	byte	if_type;	/* Type of interface		*/
	byte	if_eui64[8];	/* EUI64 address of interface	*/

	struct	ifipaddr if_ipucast[IF_NIPUCAST];/* IP ucast addresses	*/
	byte	if_nipucast;	/* No. of IP unicast addresses	*/
	struct	ifipaddr if_ipmcast[IF_NIPMCAST];/* IP mcast addresses	*/
	byte	if_nipmcast;	/* No. of IP multicast addresses*/
};

extern	struct ifentry if_tab[NIFACES];
