/* ip.h  -  Constants related to Internet Protocol version 4 (IPv4) */

#define IP_ICMPV6	58		/* ICMPv6 next header for IP	*/
#define	IP_UDP		17		/* UDP next header for IP 	*/
#define IP_IPV6		41		/* IPv6 next header for IP	*/

/* IPv6 externsion next header values */

#define IP_EXT_HBH	0		/* Hop-By-Hop ext. header	*/
#define IP_EXT_RH	43		/* Routing ext. header		*/
#define IP_EXT_FRG	44		/* Fragment ext. header		*/
#define IP_EXT_DST	60		/* Dest. options ext. header	*/

/* Option types for HBH and Destination ext. headers */

#define IP_OPT_PAD1	0		/* Pad1 option type		*/
#define IP_OPT_PADN	1		/* PadN option type		*/

#define	IP_ASIZE	16		/* Bytes in an IP address	*/
#define	IP_HDR_LEN	40		/* Bytes in an IP header	*/

#define	IP_OQSIZ	8		/* Size of IP output queue	*/

/* Queue of outgoing IP packets waiting for ipout process */

struct	iqentry	{
	int32	iqhead;			/* Index of next packet to send	*/
	int32	iqtail;			/* Index of next free slot	*/
	sid32	iqsem;			/* Semaphore that counts pkts	*/
	struct	netpacket *iqbuf[IP_OQSIZ];/* Circular packet queue	*/
};

extern	struct	iqentry	ipoqueue;	/* Network output queue		*/

extern	byte ip_llprefix[];		/* IPv6 Link local prefix	*/
extern	byte ip_loopback[];		/* IPv6 Loopback address	*/
extern	byte ip_unspec[];		/* IPv6 unspecified address	*/

#define	isipllu(x)	(memcmp((char *)(x), (char *)ip_llprefix, 8)==0)
#define isiplb(x)	(memcmp((char *)(x), (char *)ip_loopback, 16)==0)
#define isipmc(x)	((*x) == 0xff)
