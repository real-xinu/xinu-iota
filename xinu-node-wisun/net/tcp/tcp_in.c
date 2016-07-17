/* tcp_in.c  -  tcp_in */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  tcp_in  -  process an incoming TCP segment
 *------------------------------------------------------------------------
 */
void	tcp_in(
	  struct netpacket *pkt		/* Ptr to arriving packet	*/
	)
{
	int32	i;			/* Iterates through TCBs	*/
	//byte	ebcast[] = {0xff,0xff,0xff,0xff,0xff,0xff};
	int32	entry;
	uint16	len;			/* Length of the segment	*/
	struct	tcb	*tcbptr;	/* Ptr to TCB entry		*/
	int32	found = -1;

	/* Get pointers to Ethernet header, IP header and TCP header */

	len = pkt->net_iplen;

	/* Reject broadcast or multicast packets out of hand */
	/*
	if ((memcmp(pkt->net_ethdst,ebcast,ETH_ADDR_LEN)== 0) ||
				(pkt->net_ipdst == IP_BCAST)) {
		freebuf ((char *)pkt);
		return;
	}*/
/*DEBUG*///pdumph(pkt);

	/* Validate header lengths */
	/*
	if (len < (IP_HDR_LEN + TCP_HLEN(pkt)) ) {
		freebuf ((char *)pkt);
		return;
	}
	*/
	/* Insure exclusive use */

	wait (Tcp.tcpmutex);
	for (i = 0; i < Ntcp; i++) {
		tcbptr = &tcbtab[i];
		if (tcbptr->tcb_state == TCB_FREE) {

			/* Skip unused emtries */
			continue;
		} else if (tcbptr->tcb_state == TCB_LISTEN) {

			if((isipunspec(tcbptr->tcb_lip) || (!memcmp(tcbptr->tcb_lip, pkt->net_ipdst, 16))) &&
			   (pkt->net_tcpdport == tcbptr->tcb_lport)) {
			   	found = i;
				continue;
			}
			#if 0
			/* Check partial match */

			/* Accept only if the current entry	*/
			/* matches and is more specific		*/

			if (((tcbptr->tcb_lip == 0
			      && partial == -1)
			     || pkt->net_ipdst == tcbptr->tcb_lip)
			    && pkt->net_tcpdport == tcbptr->tcb_lport) {
				partial = i;
			}
			continue;
			#endif
		} else {

			/* Check for exact match */

			if ((!memcmp(pkt->net_ipsrc, tcbtab[i].tcb_rip, 16))
			    && (!memcmp(pkt->net_ipdst, tcbptr->tcb_lip, 16))
			    && (pkt->net_tcpsport == tcbptr->tcb_rport)
			    && (pkt->net_tcpdport == tcbptr->tcb_lport)) {
			    	found = i;
				break;
			}
			continue;
		}
	}

	/* See if full match found, partial match found, or none */

	#if 0
	if (complete != -1) {
		entry = complete;
	} else if (partial != -1) {
		entry = partial;
	#endif
	if(found != -1) {
		entry = found;
	} else {

		/* No match - send a reset and drop the packet */

		signal (Tcp.tcpmutex);
		tcpreset (pkt);
		freebuf ((char *)pkt);
		return;
	}

	/* Wait on mutex for TCB and release global mutex */

	wait (tcbtab[entry].tcb_mutex);
	signal (Tcp.tcpmutex);

	/* Process the segment according to the state of the TCB */

	tcpdisp (&tcbtab[entry], pkt);
	signal (tcbtab[entry].tcb_mutex);
	freebuf ((char *)pkt);

	return;
}
