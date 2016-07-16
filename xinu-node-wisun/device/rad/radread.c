/* radread.c - radread */

#include <xinu.h>

/*------------------------------------------------------------------------
 * radread  -  Read an incoming packet
 *------------------------------------------------------------------------
 */
int32	radread (
	struct	dentry *devptr,	/* Entry in device switch table	*/
	char	*buf,		/* Buffer to copy the packet in	*/
	int32	len		/* Size of the buffer		*/
	)
{
	struct	radcblk *radptr;
	struct	radpacket *pkt;
	struct	netpacket *npkt;
	byte	*hptr, *cend, *uend;
	int32	count;

	radptr = &radtab[devptr->dvminor];

	//wait(radptr->isem);
	pkt = (struct radpacket *)getbuf(radptr->inPool);
	hptr = pkt->rad_data;

	count = read(ETHER0, (char *)pkt->rad_data, RAD_PKT_SIZE);
	if(count == SYSERR) {
		freebuf((char *)pkt);
		return SYSERR;
	}
	if(pkt->rad_data[0] == 0) {
		memcpy(buf, pkt->rad_data, count+8);
		freebuf((char *)pkt);
		return count;
	}

	npkt = (struct netpacket *)buf;

	hptr += 3;
	memcpy(npkt->net_raddstpan, hptr, 2);
	hptr += 2;
	memcpy(npkt->net_raddstaddr, hptr, 8);
	hptr += 8;
	memcpy(npkt->net_radsrcpan, hptr, 2);
	hptr += 2;
	memcpy(npkt->net_radsrcaddr, hptr, 8);
	hptr += 8;

	if(radptr->_6lowpan) {
		count = _6lpread(hptr, count - 23, npkt->net_radie, npkt->net_radsrcaddr, npkt->net_raddstaddr);
		count += 23;
	}
	/*
	kprintf("%x %x\n",(*hptr)&LOWPAN_DISPATCH_MASK, LOWPAN_IPHC);
	if(((*hptr)&LOWPAN_DISPATCH_MASK) == LOWPAN_IPHC) {
		kprintf("calling raduncompress\n");
		count = raduncompress(hptr, count-23, &npkt->net_ipvtch, npkt->net_radsrcaddr, npkt->net_raddstaddr);
		count += 23;
	}
	else {
		return SYSERR;
	}*/
	/*memcpy(uend, cend, count-(cend-pkt->rad_data));
	uend += count - (cend-pkt->rad_data);
	count = uend - (byte *)buf;
	*/

	//memcpy(buf, (char *)pkt, len < count ? len : count);

	freebuf((char *)pkt);
	//signal(radptr->isem);

	return len < count ? len : count;
}
#if 0
process	rawin (void) {

	struct	netpacket *pkt;
	struct	ifentry *ifptr;
	int32	retval;

	while(TRUE) {

		pkt = getbuf(netbufpool);
		if((int32)pkt == SYSERR) {
			continue;
		}

		retval = read(RADIO0, (char *)pkt, sizeof(struct netpacket));
		if(retval == SYSERR) {
			panic("rawin: radio read failed");
		}

		if(pkt->net_ethpad[0] == 0) {
			ifptr = &if_tab[IF_ETH];
		}
		else {
			ifptr = &if_tab[IF_RADIO];
		}

		if(semcount(ifptr->isem) > 10) {
			freebuf((char *)pkt);
			continue;
		}
		ifptr->pktbuf[ifptr->tail] = pkt;
		ifptr->tail++;
		if(ifptr->tail >= 10) {
			ifptr->tail = 0;
		}
		signal(ifptr->isem);
	}

	return OK;
}
#endif
