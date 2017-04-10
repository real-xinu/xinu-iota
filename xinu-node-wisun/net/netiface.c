/* netiface.c - netiface_init */

#include <xinu.h>

/* Table of network interfaces */
struct	netiface iftab[NIFACES];

/* Until we have a radio device */
struct	radcblk radtab[1];

/*------------------------------------------------------------------------
 * netiface_init  -  Initialize all the network interfaces
 *------------------------------------------------------------------------
 */
void	netiface_init(void) {

	struct	netiface *ifptr;	/* Pointer to table entry	*/
	int	ifindex;		/* Index into the table		*/
	intmask	mask;			/* Saved interrupt mask		*/
	int32	retval;

	mask = disable();

	for(ifindex = 0; ifindex < NIFACES; ifindex++) {

		/* Get a pointer to the table entry */
		ifptr = &iftab[ifindex];

		/* Clear out the entry */
		memset((char *)ifptr, NULLCH, sizeof(struct netiface));

		/* Set the state as DOWN */
		ifptr->if_state = IF_DOWN;

		/* Initialize the input queue for this interface */
		ifptr->if_iqtail = ifptr->if_iqhead = 0;
		ifptr->if_iqsem = semcreate(0);

		/* Initialize the ND related fields */
		ifptr->if_nd_reachtime = ND_REACHABLE_TIME;
		ifptr->if_nd_retranstime = ND_RETRANS_TIMER;

		if(ifindex == 0) { /* First is the ethernet interface */

			/* Set the interface type */
			ifptr->if_type = IF_TYPE_ETH;

			/* Set the device in the interface */
			ifptr->if_dev = ETHER0;

			/* Set the hardware addresses */
			ifptr->if_halen = IF_HALEN_ETH;
			memcpy(ifptr->if_hwucast, ethertab[0].devAddress,
					IF_HALEN_ETH);
			memset(ifptr->if_hwbcast, 0xff, IF_HALEN_ETH);

			/* Generate a Link-local IPv6 address */
			ifptr->if_nipucast = 1;
			memcpy(ifptr->if_ipucast[0].ipaddr, ip_llprefix, 16);
			memcpy(ifptr->if_ipucast[0].ipaddr+8, ifptr->if_hwucast, 3);
			ifptr->if_ipucast[0].ipaddr[11] = 0xff;
			ifptr->if_ipucast[0].ipaddr[12] = 0xfe;
			memcpy(ifptr->if_ipucast[0].ipaddr+13, ifptr->if_hwucast+3, 3);
			if(ifptr->if_hwucast[0] & 0x02) {
				ifptr->if_ipucast[0].ipaddr[8] &= 0xfd;
			}
			else {
				ifptr->if_ipucast[0].ipaddr[8] |= 0x02;
			}

			retval = nd_newipucast(ifindex, ifptr->if_ipucast[0].ipaddr);
			if(retval == SYSERR) {
				kprintf("Cannot assign Link-local IP address to interface %d\n", ifindex);
				panic("");
			}

			ifptr->if_state = IF_UP;
		}
		else if(ifindex == 1) { /* Second is the radio interface */

			/* Set the interface type */
			ifptr->if_type = IF_TYPE_RAD;

			/* Set the device in the interface */
			//ifptr->if_dev = &radtab[0];

			/* Set the hardware addresses */
			ifptr->if_halen = IF_HALEN_RAD;
			memcpy(ifptr->if_hwucast, radtab[0].devAddress,
					IF_HALEN_RAD);
			memset(ifptr->if_hwbcast, 0xff, IF_HALEN_RAD);

			/* Genarate a Link-local IPv6 address */
			ifptr->if_nipucast = 1;
			memcpy(ifptr->if_ipucast[0].ipaddr, ip_llprefix, 16);
			memcpy(ifptr->if_ipucast[0].ipaddr+8, ifptr->if_hwucast, 8);
			if(ifptr->if_hwucast[0] & 0x02) {
				ifptr->if_ipucast[0].ipaddr[8] &= 0xfd;
			}
			else {
				ifptr->if_ipucast[0].ipaddr[8] |= 0x02;
			}

			ifptr->if_state = IF_UP;
		}

		kprintf("Stateless Address Autoconfiguration performed on interface %d.\n", ifindex);
		kprintf("IP address: "); ip_printaddr(ifptr->if_ipucast[0].ipaddr); kprintf("\n");
	}

	restore(mask);
}
