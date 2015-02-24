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

		if(iface == 0) {
			ifptr->if_type = IF_TYPE_RADIO;
			ifptr->if_dev = RADIO0;

			control(ifptr->if_dev, RAD_CTRL_GET_EUI64, (uint32)ifptr->if_eui64, 0);

			memcpy(ifptr->if_ipucast[0].ipaddr, ip_llprefix, 8);
			memcpy(&ifptr->if_ipucast[0].ipaddr[8], ifptr->if_eui64, 8);
			ifptr->if_nipucast = 1;

			ifptr->if_state = IF_UP;
		}
	}

	restore(mask);
}
