/* netiface.c - netiface_init */

#include <xinu.h>

struct	ifentry if_tab[NIFACES];

/*------------------------------------------------------------------------
 * netiface_init  -  Initialize the network interfaces
 *------------------------------------------------------------------------
 */
void	netiface_init ()
{
	int32	iface;		/* Index in the interface table	*/
	struct	ifentry *ifptr;	/* Pointer to interface entry	*/

	for(iface = 0; iface < NIFACES; iface++) {

		/* Get pointer to interface entry */

		ifptr = &if_tab[iface];

		/* Set the whole entry to zeros */

		memset((char *)ifptr, NULLCH, sizeof(struct ifentry));

		if(iface == 0) { /* 802.15.4 interface */

			ifptr->if_dev = ETHER0;

			control(ETHER0, ETH_CTRL_GET_MAC, (int32)ifptr->if_eui64, 0);

			memcpy(ifptr->if_ipucast[0].ipaddr, ipllprefix, 16);
			memcpy(&ifptr->if_ipucast[0].ipaddr[8], ifptr->if_eui64, 8);
			ifptr->if_ipucast[0].preflen = 64;
			ifptr->if_nipucast = 1;
		}
	}
}
