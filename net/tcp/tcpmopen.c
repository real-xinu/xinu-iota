/* tcpmopen.c  -  checktuple, tcpmopen */

#include <xinu.h>
#include <string.h>

/*------------------------------------------------------------------------
 *  ckecktuple  -  Verify that a TCP connection is not already in use by
 *			checking (src IP, src port, Dst IP, Dst port)
 *------------------------------------------------------------------------
 */
static	int32	checktuple (
		  byte		lip[16],/* Local IP address		*/
		  uint16	lport,	/* Local TCP port number	*/
		  byte		rip[16],/* Remote IP address		*/
		  uint16	rport	/* Remote TCP port number	*/
		)
{
	int32	i;			/* Walks though TCBs		*/
	struct	tcb *tcbptr;		/* Ptr to TCB entry 		*/

	/* Check each TCB that is open */

	for (i = 0; i < Ntcp; i++) {
		tcbptr = &tcbtab[i];
		if (tcbptr->tcb_state != TCB_FREE
		    && (!memcmp(tcbptr->tcb_lip, lip, 16))
		    && (!memcmp(tcbptr->tcb_rip, rip, 16))
		    && tcbptr->tcb_lport == lport
		    && tcbptr->tcb_rport == rport) {
			return SYSERR;
		}
	}
	return OK;
}

/*------------------------------------------------------------------------
 *  tcp_register  -  Open a master the TCP device and receive the ID of a
 *			TCP slave device for the connection
 *------------------------------------------------------------------------
 */
int32	tcp_register (
	int32	iface,
	byte	ip[16],
	uint16	port,
	int32	active
	)
{
	char		*spec;		/* IP:port as a string		*/
	struct tcb	*tcbptr;	/* Ptr to TCB			*/
	byte		lip[16];	/* Local IP address		*/
	int32		i;		/* Walks through TCBs		*/
	int32		state;		/* Connection state		*/
	int32		slot;		/* Slot in TCB table		*/

	/* Parse "X:machine:port" string and set variables, where	*/
	/*	X	- either 'a' or 'p' for "active" or "passive"	*/
	/*	machine	- an IP address in dotted decimal		*/
	/*	port	- a protocol port number			*/
/*
	spec = (char *)arg1;
	if (tcpparse (spec, &ip, &port, &active) == SYSERR) {
		return SYSERR;
	}
*/
	/* Obtain exclusive use, find free TCB, and clear it */

	wait (Tcp.tcpmutex);
	for (i = 0; i < Ntcp; i++) {
		if (tcbtab[i].tcb_state == TCB_FREE)
			break;
	}
	if (i >= Ntcp) {
		signal (Tcp.tcpmutex);
		return SYSERR;
	}
	tcbptr = &tcbtab[i];
	tcbclear (tcbptr);
	slot = i;

	/* Either set up active or passive endpoint */

	if (active) {

		/* Obtain local IP address from interface */

		if(isipllu(ip)) {
			if(iface == -1) {
				signal(Tcp.tcpmutex);
				return SYSERR;
			}
			else {
				memcpy(lip, if_tab[iface].if_ipucast[0].ipaddr, 16);
			}
		}
		else {
			//TODO
		}

		//lip = NetData.ipucast;

		/* Allocate receive buffer and initialize ptrs */

		tcbptr->tcb_rbuf = (char *)getmem (65535);
		if (tcbptr->tcb_rbuf == (char *)SYSERR) {
			signal (Tcp.tcpmutex);
			return SYSERR;
		}
		tcbptr->tcb_rbsize = 65535;
		tcbptr->tcb_sbuf = (char *)getmem (65535);
		if (tcbptr->tcb_sbuf == (char *)SYSERR) {
			freemem ((char *)tcbptr->tcb_rbuf, 65535);
			signal (Tcp.tcpmutex);
			return SYSERR;
		}
		tcbptr->tcb_sbsize = 65535;

		/* The following will always succeed because	*/
		/*   the iteration covers at least Ntcp+1 port	*/
		/*   numbers and there are only Ntcp slots	*/

		for (i = 0; i < Ntcp + 1; i++) {
			if (checktuple (lip, Tcp.tcpnextport,
					      ip, port) == OK) {
				break;
			}
			Tcp.tcpnextport++;
			if (Tcp.tcpnextport > 63000)
				Tcp.tcpnextport = 33000;
		}

		memcpy(tcbptr->tcb_lip, lip, 16);
		tcbptr->tcb_iface = iface;

		/* Assign next local port */


		tcbptr->tcb_lport = Tcp.tcpnextport++;
		if (Tcp.tcpnextport > 63000) {
			Tcp.tcpnextport = 33000;
		}
		memcpy(tcbptr->tcb_rip, ip, 16);
		tcbptr->tcb_rport = port;
		tcbptr->tcb_snext = tcbptr->tcb_suna = tcbptr->tcb_ssyn = 1;
		tcbptr->tcb_state = TCB_SYNSENT;
		wait (tcbptr->tcb_mutex);
		tcbref (tcbptr);
		signal (Tcp.tcpmutex);

		tcbref (tcbptr);
		mqsend (Tcp.tcpcmdq, TCBCMD(tcbptr, TCBC_SEND));
		while (tcbptr->tcb_state != TCB_CLOSED
		       && tcbptr->tcb_state != TCB_ESTD
		       && tcbptr->tcb_state != TCB_CWAIT) {
			tcbptr->tcb_readers++;
			signal (tcbptr->tcb_mutex);
			kprintf("tcp_register: waiting for rblock, state %d\n", tcbptr->tcb_state);
			wait (tcbptr->tcb_rblock);
			kprintf("tcp_register: got rblock\n");
			wait (tcbptr->tcb_mutex);
			kprintf("tcp_register: got mutex\n");
		}
		if ((state = tcbptr->tcb_state) == TCB_CLOSED) {
			tcbunref (tcbptr);
		}
		signal (tcbptr->tcb_mutex);
		return (state == TCB_CLOSED ? SYSERR : slot);

	} else {  /* Passive connection */
		for (i = 0; i < Ntcp; i++) {
			if (tcbtab[i].tcb_state == TCB_LISTEN
			    && tcbtab[i].tcb_lport == port) {

				if(isipunspec(tcbtab[i].tcb_lip)) {
					signal(Tcp.tcpmutex);
					return SYSERR;
				}

				if(!memcmp(tcbtab[i].tcb_lip, ip, 16)) {
					signal(Tcp.tcpmutex);
					return SYSERR;
				}

				#if 0
				/* Duplicates prior connection */
 
				signal (Tcp.tcpmutex);
				return SYSERR;
				#endif
			}
		}

		/* New passive endpoint - fill in TCB */

		memcpy(tcbptr->tcb_lip, ip, 16);
		tcbptr->tcb_lport = port;
		tcbptr->tcb_state = TCB_LISTEN;
		tcbref (tcbptr);
		signal (Tcp.tcpmutex);

		/* Return slave device to caller */

		return slot;
	}
}
