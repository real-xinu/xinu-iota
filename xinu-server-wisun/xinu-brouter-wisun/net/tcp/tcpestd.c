/* tcpestd.c  -  tcpestd */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  tcpestd  -  Handle an arriving segment for the Established state
 *------------------------------------------------------------------------
 */
int32	tcpestd(
	  struct tcb	*tcbptr,	/* Ptr to a TCB			*/
	  struct netpacket *pkt		/* Ptr to a packet		*/
	)
{
	/* First, process data in the segment */

	tcpdata (tcbptr, pkt);

	/* If a FIN has been seen, move to Close-Wait */

	if (tcbptr->tcb_flags & TCBF_FINSEEN
	    && SEQ_CMP(tcbptr->tcb_rnext, tcbptr->tcb_rfin) > 0) {
		tcbptr->tcb_state = TCB_CWAIT;
	}

	/* Transmit a response immediately */

	tcpxmit (tcbptr);

	return OK;
}
