/* nd.c */

#include <xinu.h>

struct	nd_nce nd_ncache[ND_NC_SLOTS];
byte	nd_msgbuf1[500];
byte	nd_msgbuf2[500];

/*------------------------------------------------------------------------
 * nd_init  -  Initialize Neighbor Discovery related information
 *------------------------------------------------------------------------
 */
void	nd_init (
	int32	iface	/* Interface index	*/
	)
{
	struct	ifentry *ifptr;	/* Pointer to interface entry	*/
	struct	nd_info *ndptr;	/* Pointer tp ND information	*/
	intmask	mask;		/* Saved interrupt mask		*/
	int32	i;

	if((iface < 0) || (iface >= NIFACES)) {
		return;
	}

	mask = disable();

	ifptr = &if_tab[iface];

	memset((char *)&ifptr->if_ndData, 0, sizeof(ifptr->if_ndData));

	ndptr = &ifptr->if_ndData;
	ndptr->isrouter = TRUE;
	ndptr->sendadv = FALSE;
	ndptr->currhl = 255;

	for(i = 0; i < ND_NC_SLOTS; i++) {
		memset((char *)&nd_ncache[i], 0, sizeof(struct nd_nce));
		nd_ncache[i].state = ND_NCE_FREE;
	}

	resume(create(nd_in, ND_PROCSSIZE, ND_PROCPRIO, "nd_in", 1, iface));
}

/*------------------------------------------------------------------------
 * nd_in  -  ND process to handle incoming ND messages
 *------------------------------------------------------------------------
 */
process	nd_in (
	int32	iface	/* Interface index	*/
	)
{
	struct	ipinfo ipdata;	/* IP information	*/
	int32	slots[4];	/* ICMP slots		*/
	byte	*msgbuf;	/* Message buffer	*/
	uint32	buflen;		/* Buffer length	*/
	int32	retval;		/* Return value 	*/

	if((iface < 0) || (iface >= NIFACES)) {
		return SYSERR;
	}

	slots[0] = icmp_register(iface, ip_unspec, ip_unspec, ICMP_TYPE_RTRSOL, 0);
	slots[1] = icmp_register(iface, ip_unspec, ip_unspec, ICMP_TYPE_RTRADV, 0);
	slots[2] = icmp_register(iface, ip_unspec, ip_unspec, ICMP_TYPE_NBRSOL, 0);
	slots[3] = icmp_register(iface, ip_unspec, ip_unspec, ICMP_TYPE_NBRADV, 0);

	if((slots[0] == SYSERR) ||
	   (slots[1] == SYSERR) ||
	   (slots[2] == SYSERR) ||
	   (slots[3] == SYSERR)) {
	   	panic("nd_in: Error while registering icmp slots");
	}

	while(TRUE) {
		msgbuf = nd_msgbuf1;
		buflen = 500;
		retval = icmp_recvnaddr(slots,	/* ICMP slots to listen on	*/
					4,	/* No. of ICMP slots		*/
					msgbuf, /* Message buffer		*/
					&buflen,/* Buffer length		*/
					3000,	/* Timeout in ms		*/
					&ipdata	/* IP information		*/
					);
		if(retval == SYSERR) {
			panic("nd_in: error in icmp_recvnaddr");
		}
		if(retval == TIMEOUT) {
			continue;
		}
		if(retval == slots[0]) {
			kprintf("Incoming rtrsol\n");
		}
		else if(retval == slots[1]) {
			kprintf("Incoming rtradv\n");
		}
		else if(retval == slots[2]) {
			kprintf("Incoming nbrsol\n");
		}
		else if(retval == slots[3]) {
			kprintf("Incoming nbradv\n");
		}
		else {
			panic("nd_in: Unknown message type\n");
		}
	}

	return OK;
}
