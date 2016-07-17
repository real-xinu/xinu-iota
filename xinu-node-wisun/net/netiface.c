/* netiface.c - netiface_init */

#include <xinu.h>

struct	ifentry if_tab[NIFACES];

/*------------------------------------------------------------------------
 * netiface_init  -  Initializes all the network interfaces
 *------------------------------------------------------------------------
 */
void	netiface_init (void)
{
	int32	iface;		/* Interface number	*/
	struct	ifentry *ifptr;	/* Interface pointer	*/
	intmask	mask;		/* Saved interrupt mask	*/

	/* Ensure only one process accesses the interface table */

	mask = disable();

	for(iface = 0; iface < NIFACES; iface++) {
		ifptr = &if_tab[iface];

		memset((char *)ifptr, NULLCH, sizeof(struct ifentry));
		ifptr->if_state = IF_DOWN;

		if(iface == IF_RADIO) {

			/* Set the type and device */

			ifptr->if_type = IF_TYPE_RADIO;
			ifptr->if_dev = RADIO0;

			/* Set the interface HW addresses */

			control(ifptr->if_dev, RAD_CTRL_GET_EUI64, (uint32)ifptr->if_hwucast, 0);
			memset(ifptr->if_hwbcast, 0xff, 8);
			ifptr->if_halen = 8;

			/* Generate the link local IP address */

			memcpy(ifptr->if_ipucast[0].ipaddr, ip_llprefix, 8);
			memcpy(&ifptr->if_ipucast[0].ipaddr[8], ifptr->if_hwucast, 8);
			ifptr->if_nipucast = 1;

			/* Initialize ND for this interface */

			nd_init(iface);

			ifptr->if_state = IF_UP;
		}
		else if(iface == IF_ETH) {

			/* Set the type and device */

			ifptr->if_type = IF_TYPE_ETH;
			ifptr->if_dev = ETHER0;

			/* Set the interface HW addresses */

			control(ifptr->if_dev, ETH_CTRL_GET_MAC, (uint32)ifptr->if_hwucast, 0);
			memset(ifptr->if_hwbcast, 0xff, 6);
			ifptr->if_halen = 6;

			/* Generate the link local IP address */

			memcpy(ifptr->if_ipucast[0].ipaddr, ip_llprefix, 16);
			memcpy(&ifptr->if_ipucast[0].ipaddr[8], ifptr->if_hwucast, 3);
			ifptr->if_ipucast[0].ipaddr[11] = 0xff;
			ifptr->if_ipucast[0].ipaddr[12] = 0xfe;
			memcpy(&ifptr->if_ipucast[0].ipaddr[13], &ifptr->if_hwucast[3], 3);
			if(ifptr->if_ipucast[0].ipaddr[8]&0x02) {
				ifptr->if_ipucast[0].ipaddr[8] &= 0xfd;
			}
			else {
				ifptr->if_ipucast[0].ipaddr[8] |= 0x02;
			}
			ifptr->if_nipucast = 1;

			/* Join the solicited node multicast address */

			memcpy(ifptr->if_ipmcast[0].ipaddr, ip_solmc, 16);
			memcpy(&ifptr->if_ipmcast[0].ipaddr[13], &ifptr->if_ipucast[0].ipaddr[13], 3);
			ifptr->if_nipmcast = 1;

			/* Initialize ND for this interface */

			nd_init(iface);

			ifptr->if_state = IF_UP;
		}

		/* For emulation only */
		ifptr->head = ifptr->tail = ifptr->count = 0;
		ifptr->isem = semcreate(0);
	}

	restore(mask);
}
