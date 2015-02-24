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

	wait(radptr->isem);
	pkt = (struct radpacket *)getbuf(radptr->inPool);
	hptr = pkt->rad_data;

	count = read(ETHER0, (char *)pkt->rad_data, RAD_PKT_SIZE);
	if(count == SYSERR) {
		return SYSERR;
	}
	kprintf("incoming pkt.. %d\n", count);
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
	int32	i;
	kprintf("IN: ");
	for(i = 0; i < count; i++) {
		kprintf("%x ", (byte)buf[i]);
	}
	kprintf("\n");

	//memcpy(buf, (char *)pkt, len < count ? len : count);

	freebuf(pkt);
	signal(radptr->isem);

	return len < count ? len : count;
}
