/* radwrite - radwrite */

#include <xinu.h>
extern int32 _6lpwrite(void *, void *, byte *, byte *);
/*------------------------------------------------------------------------
 * radwrite  -  Transmit a packet
 *------------------------------------------------------------------------
 */
int32	radwrite (
	struct	dentry *devptr,	/* Entry in device switch table	*/
	char	*buf,		/* Pointer to packet buffer	*/
	int32	len		/* Size of the buffer		*/
	)
{
	struct	radcblk *radptr;
	struct	radpacket *outpkt;
	struct	netpacket *inpkt;
	byte	*cstart;
	byte	*hptr;
	byte	*cend, *uend;
	byte	*fcs1, *fcs2;
	static	byte seqnum = 0;
	uint32	hdrlen;
	uint32	unclen;
	int32	retval;
	intmask	mask;
	kprintf("radwrite..\n");
	mask = disable();

	radptr = &radtab[devptr->dvminor];

	wait(radptr->osem);
	outpkt = (struct radpacket *)getbuf(radptr->outPool);
	inpkt = (struct netpacket *)buf;

	hdrlen = 23;	/* FCS + Seq + dst pan eui + src pan eui */
	cstart = outpkt->rad_data + hdrlen;

	/* Leave two bytes at the start for ethernet driver */
	cstart += 2;

	if(radptr->_6lowpan) {
		retval = _6lpwrite(inpkt, cstart, inpkt->net_radsrcaddr, inpkt->net_raddstaddr);
		if(retval == SYSERR) {
			restore(mask);
			return SYSERR;
		}
		kprintf("radwrite: _6lpwrite returns %d\n", retval);
		cend = cstart + retval;
	}
	/*
	inpkt->net_radtype = RAD_TYPE_DATA;
	if(inpkt->net_radtype == RAD_TYPE_DATA) {
		retval = radcompress(radptr, inpkt, cstart, &uend, &cend);
		if(retval == SYSERR) {
			restore(mask);
			return SYSERR;
		}
	}
	else {
		return SYSERR; //for now
	}
	
	unclen = 40 + ntohs(inpkt->net_iplen) - (uend - (byte *)&inpkt->net_ipvtch);

	//kprintf("%d bytes compressed to %d\n", (uend-(byte*)&inpkt->net_ipvtch), cend-cstart);
	memcpy(cend, uend, unclen);
	cend += unclen;
	*/
	outpkt->rad_data[0] = inpkt->net_raddstaddr[7];
	outpkt->rad_data[1] = inpkt->net_radsrcaddr[7];
	hptr = outpkt->rad_data + 2;
	fcs1 = hptr++;
	fcs2 = hptr++;
	*fcs1 = 0;
	*fcs1 |= (inpkt->net_radtype & 0x7);
	*fcs2 = 0;
	//*fcs2 |= (1 << RAD_FCS_SEQ);
	*hptr++ = seqnum++;

	*fcs2 |= (0x03 << RAD_FCS_DAM);
	memcpy(hptr, inpkt->net_raddstpan, 2);
	hptr += 2;
	memcpy(hptr, inpkt->net_raddstaddr, 8);
	hptr += 8;

	*fcs2 |= (0x02 << RAD_FCS_VER);

	*fcs2 |= (0x03 << RAD_FCS_SAM);
	memcpy(hptr, inpkt->net_radsrcpan, 2);
	hptr += 2;
	memcpy(hptr, inpkt->net_radsrcaddr, 8);
	hptr += 8;

	write(ETHER0, (char *)outpkt->rad_data, cend-outpkt->rad_data);

	freebuf(outpkt);
	signal(radptr->osem);

	restore(mask);
	return len;
}
