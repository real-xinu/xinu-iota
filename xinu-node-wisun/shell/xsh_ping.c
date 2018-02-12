/* xsh_ping.c - xsh_ping */

#include <xinu.h>
#include <stdio.h>
#include <string.h>

/*------------------------------------------------------------------------
 * xsh_ping - shell command to ping a remote host
 *------------------------------------------------------------------------
 */
shellcmd xsh_ping(int nargs, char *args[])
{
	byte	ipaddr[16];		/* IP address in binary		*/
	int32	retval;			/* return value			*/
	int32	slot;			/* Slot in ICMP to use		*/
	//static	int32	seq = 0;/* sequence number		*/
	char	buf[56];		/* buffer of chars		*/
	int32	i;			/* index into buffer		*/
	int32	n;			/* No. if echo packets		*/

	/* For argument '--help', emit help about the 'ping' command	*/

	if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
		printf("Use: %s  address\n\n", args[0]);
		printf("Description:\n");
		printf("\tUse ICMP Echo to ping a remote host\n");
		printf("Options:\n");
		printf("\t--help\t display this help and exit\n");
		printf("\taddress\t an IP address in dotted decimal\n");
		return 0;
	}

	/* Check for valid number of arguments */

	if (nargs < 2 || nargs > 3) {
		fprintf(stderr, "%s: invalid arguments\n", args[0]);
		fprintf(stderr, "Try '%s --help' for more information\n",
				args[0]);
		return 1;
	}

	retval = colon2ip(args[1], ipaddr);
	if(retval == SYSERR) {
		fprintf(stderr, "%s invalid IP address\n");
		return 1;
	}

	slot = icmp_register(ICMP_TYPE_ERP, 0, ipaddr, ip_unspec, -1);
	if(slot == SYSERR) {
		fprintf(stderr, "ICMP registration failed\n");
		return 1;
	}

	n = 1;
	if(nargs == 3) {
		n = atoi(args[2]);
	}

	for(i = 0; i < n; i++) {

		retval = icmp_send(ICMP_TYPE_ERQ, 0, ip_unspec, ipaddr, buf, sizeof(buf), 0);
		if(retval == SYSERR) {
			fprintf(stderr, "ICMP Sending Failed\n");
			icmp_release(slot);
			return 1;
		}

		retval = icmp_recv(slot, buf, sizeof(buf), 3000);
		if(retval == SYSERR) {
			fprintf(stderr, "ICMP receiving Failed\n");
			icmp_release(slot);
			return 1;
		}

		if(retval == TIMEOUT) {
			fprintf(stderr, "ICMP Echo Request Timed out\n");
			continue;
		}

		if(retval != sizeof(buf)) {
			fprintf(stderr, "Expected %d bytes, got back %d\n", sizeof(buf), retval);
			continue;
		}

		fprintf(stderr, "Response from %s, seq = %d\n", args[1], i);

		if(i < (n-1)) {
			sleep(1);
		}
	}

	icmp_release(slot);

	return 0;
}
