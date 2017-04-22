/* xsh_ipaddr.c - xsh_ipaddr */

#include <xinu.h>
#include <string.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * xsh_ipaddr - obtain and print the IP address, subnet mask and default
 *			router address for each interface that's up
 *------------------------------------------------------------------------
 */
shellcmd xsh_ipaddr(int nargs, char *args[]) {

	struct	netiface *ifptr;
	int32	i, j;

	/* Output info for '--help' argument */

	if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
		printf("Usage: %s\n\n", args[0]);
		printf("Description:\n");
		printf("\tDisplays IP address information\n");
		printf("Options:\n");
		printf("\t-f\tforce a new DHCP request");
		printf("\t--help\tdisplay this help and exit\n");
		return OK;
	}

	/* Check argument count */

	if (nargs > 2) {
		fprintf(stderr, "%s: too many arguments\n", args[0]);
		fprintf(stderr, "Try '%s --help' for more information\n",
			args[0]);
		return SYSERR;
	}

	kprintf("Node ID: %d\n", info.nodeid);
	kprintf("Link info: ");
	for(i = 0; i < 46; i++) {
		kprintf("%02x ", info.link_info[i].probloss);
	}
	kprintf("\n");
	kprintf("Assign times: ");
	for(i = 0; i < info.ntimes; i++) {
		kprintf("%d ", info.assign_times[i]);
	}
	kprintf("\n");

	for(i = 0; i < NIFACES; i++) {

		ifptr = &iftab[i];

		if(ifptr->if_state == IF_DOWN) {
			kprintf("Interface %d Down\n", i);
			continue;
		}

		kprintf("Interface %d\n", i);
		kprintf("  Unicast IP addresses:\n");
		for(j = 0; j < ifptr->if_nipucast; j++) {
			kprintf("    ");
			ip_printaddr(ifptr->if_ipucast[j].ipaddr);
			kprintf(" Time %d\n", ifptr->if_iptimes[j]);
		}
		kprintf("  Multicast IP addresses:\n");
		for(j = 0; j < ifptr->if_nipmcast; j++) {
			kprintf("    ");
			ip_printaddr(ifptr->if_ipmcast[j].ipaddr);
			kprintf("\n");
		}
		kprintf("\n");
	}

	return 0;
}
