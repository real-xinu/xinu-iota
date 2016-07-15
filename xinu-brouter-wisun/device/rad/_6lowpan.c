/* _6lowpan.c  -  _6lpwrite, _6lpread */

#include <xinu.h>

/*------------------------------------------------------------------------
 * _6lpwrite  -  Perform header compression and fragmentation
 *------------------------------------------------------------------------
 */
int32	_6lpwrite (
	void	*inptr,		/* Pointer to input buffer	*/
	void	*outptr,	/* Pointer to output buffer	*/
	byte	*encsrciid,	/* Encapsulating src IID	*/
	byte	*encdstiid	/* Encapsulating dst IID	*/
	)
{
	struct	netpacket *inpkt;
	byte	*cptr, *uptr;
	byte	*iphc1, *iphc2, *ipnhc;
	byte	nxthdr, optlen;
	byte	compress = TRUE;
	uint16	iplen, remain;
	int32	retval;

	inpkt = (struct netpacket *)inptr;
	ip_ntoh(inpkt);
	iplen = inpkt->net_iplen;

	/* Packet must be IPv6 packet */

	if((inpkt->net_ipvtch & 0xf0) != 0x60) {
		return SYSERR;
	}

	cptr = (byte *)outptr;
	uptr = (byte *)inptr;

	/* Pointers to LOWPAN_IPHC bytes */

	iphc1 = cptr++;
	iphc2 = cptr++;

	*iphc1 = LOWPAN_IPHC;

	/* Traffic class = 0, for now */

	*iphc1 |= (0x03 << LOWPAN_IPHC_TF);

	if(LOWPAN_IPNHC_CHECK(inpkt->net_ipnh)) {
		*iphc1 |= (0x01 << LOWPAN_IPNHC_NH);
		nxthdr = inpkt->net_ipnh;
	}
	else {
		*cptr++ = inpkt->net_ipnh;
		compress = FALSE;
	}
	/*
	switch(inpkt->net_ipnh) {
	 case IP_HBH:
	 case IP_IPV6:
	 	*iphc1 |= (0x01 << LOWPAN_IPHC_NH);
		break;
	 default:
	 	*cptr++ = inpkt->net_ipnh;
		compress = FALSE;
	}
	*/
	/* Hop limit */

	switch(inpkt->net_iphl) {
	 case 1:
	 	*iphc1 |= (0x01 << LOWPAN_IPHC_HL);
		break;
	 case 64:
	 	*iphc1 |= (0x02 << LOWPAN_IPHC_HL);
		break;
	 case 255:
	 	*iphc1 |= (0x03 << LOWPAN_IPHC_HL);
		break;
	 default:
	 	*cptr++ = inpkt->net_iphl;
	}

	/* For now, CID = SAC = M = DAC = 0 */

	/* Compress the IPc6 source */

	if(isipllu(inpkt->net_ipsrc)) {
		if(!memcmp(encsrciid, &inpkt->net_ipsrc[8], 8)) {
			*iphc2 |= (0x03 << LOWPAN_IPHC_SAM);
		}
		else {
			*iphc2 |= (0x01 << LOWPAN_IPHC_SAM);
			memcpy(cptr, &inpkt->net_ipsrc[8], 8);
			cptr += 8;
		}
	}
	else {
		memcpy(cptr, inpkt->net_ipsrc, 16);
		cptr += 16;
	}

	/* Compress the IPv6 destination */

	if(isipllu(inpkt->net_ipdst)) {
		if(!memcmp(encdstiid, &inpkt->net_ipdst[8], 8)) {
			*iphc2 |= (0x03 << LOWPAN_IPHC_DAM);
		}
		else {
			*iphc2 |= (0x01 << LOWPAN_IPHC_DAM);
			memcpy(cptr, &inpkt->net_ipdst[8], 8);
			cptr += 8;
		}
	}
	else {
		memcpy(cptr, inpkt->net_ipdst, 16);
		cptr += 16;
	}
	uptr = inpkt->net_ipdata;

	/* Now compress the extension headers */

	while(compress) {

		ipnhc = cptr++;
		*ipnhc = LOWPAN_IPNHC;

		switch(nxthdr) {

	  	 case IP_HBH:
	  		*ipnhc |= (0 << LOWPAN_IPNHC_EID);
			nxthdr = *uptr++;
			if(LOWPAN_IPNHC_CHECK(nxthdr)) {
				*ipnhc |= (0x01 << LOWPAN_IPNHC_NH);
			}
			else {
				*cptr++ = nxthdr;
				compress = FALSE;
			}
			optlen = (*uptr++) * 8 + 6;
			*cptr++ = optlen;
			memcpy(cptr, uptr, optlen);
			cptr += optlen;
			uptr += optlen;
		
		 case IP_IPV6:
		 	*ipnhc |= (7 << LOWPAN_IPNHC_EID);
			retval = _6lpwrite(uptr, cptr, &inpkt->net_ipsrc[8], &inpkt->net_ipdst[8]);
			if(retval == SYSERR) {
				return SYSERR;
			}
			return (cptr - (byte *)outptr) + retval;

		 default:
		 	return SYSERR;
		}
	}

	remain = iplen - (uptr - inpkt->net_ipdata);
	kprintf("_6lpwrite: remaining %d bytes\n", remain);
	memcpy(cptr, uptr, remain);
	cptr += remain;
	return (cptr - (byte *)outptr);
}

/*------------------------------------------------------------------------
 * _6lpread  -  Perform uncompression on incoming packet
 *------------------------------------------------------------------------
 */
int32	_6lpread (
	void	*inptr,		/* Compressed packet ptr	*/
	uint32	inlen,		/* No. of bytes in input buffer	*/
	void	*outptr,	/* Pointer to uncompressed pkt	*/
	byte	*encsrciid,	/* Encapsulating src IID	*/
	byte	*encdstiid	/* Encapsulatinf dst IID	*/
	)
{
	struct	iphdr *iphdr;
	byte	*cptr, *uptr, *prevnh;
	byte	iphc1, iphc2, ipnhc;
	byte	dispatch, optlen;
	uint16	iplen, remain;
	bool8	uncompress = TRUE;

	cptr = (byte *)inptr;
	uptr = (byte *)outptr;
	iphdr = (struct iphdr *)outptr;

	dispatch = *cptr++;
	kprintf("_6lpread: cptr inc 1\n");

	if((dispatch & LOWPAN_DISPATCH_MASK) != LOWPAN_IPHC) {
		return SYSERR;
	}

	iphdr->net_ipvtch = 0x60;
	iphdr->net_iptclflh = 0;
	iphdr->net_ipfll = 0;
	iphdr->net_iplen = 0;

	iphc1 = dispatch;
	iphc2 = *cptr++;
	kprintf("_6lpread: cptr inc 2\n");
	kprintf("_6lpread: iphc1 %x, iphc2 %x\n", iphc1, iphc2);
	if((iphc1 >> LOWPAN_IPHC_NH) & 0x01) {
		prevnh = &iphdr->net_ipnh;
	}
	else {
		iphdr->net_ipnh = *cptr++;
		kprintf("_6lpread: cptr inc 3\n");
		uncompress = FALSE;
	}

	switch((iphc1 >> LOWPAN_IPHC_HL) & 0x03) {

	 case 0:
	 	iphdr->net_iphl = *cptr++;
		kprintf("_6lpread: cptr inc 4\n");
		break;
	 case 1:
	 	iphdr->net_iphl = 1;
		break;
	 case 2:
	 	iphdr->net_iphl = 64;
		break;
	 case 3:
	 	iphdr->net_iphl = 255;
	}

	/* Assume for now, SAC = DAC = M = 0 */

	switch((iphc2 >> LOWPAN_IPHC_SAM) & 0x03) {

	 case 0:
	 	memcpy(iphdr->net_ipsrc, cptr, 16);
		kprintf("_6lpread: cptr inc 5\n");
		cptr += 16;
		break;
	 case 1:
	 	memcpy(iphdr->net_ipsrc, ip_llprefix, 8);
		memcpy(&iphdr->net_ipsrc[8], cptr, 8);
		kprintf("_6lpread: cptr inc 6\n");
		cptr += 8;
		break;
	 case 2:
	 	memcpy(iphdr->net_ipsrc, ip_llprefix, 8);
		memset(&iphdr->net_ipsrc[8], 0, 8);
		iphdr->net_ipsrc[11] = 0xff;
		iphdr->net_ipsrc[12] = 0xfe;
		kprintf("_6lpread: cptr inc 7\n");
		iphdr->net_ipsrc[14] = *cptr++;
		iphdr->net_ipsrc[15] = *cptr++;
		break;
	 case 3:
	 	memcpy(iphdr->net_ipsrc, ip_llprefix, 8);
		memcpy(&iphdr->net_ipsrc[8], encsrciid, 8);
		break;
	}

	switch((iphc2 >> LOWPAN_IPHC_DAM) & 0x03) {

	 case 0:
	 	memcpy(iphdr->net_ipdst, cptr, 16);
		kprintf("_6lpread: cptr inc 8\n");
		cptr += 16;
		break;
	 case 1:
	 	memcpy(iphdr->net_ipdst, ip_llprefix, 8);
		memcpy(&iphdr->net_ipdst[8], cptr, 8);
		kprintf("_6lpread: cptr inc 9\n");
		cptr += 8;
		break;
	 case 2:
	 	memcpy(iphdr->net_ipdst, ip_llprefix, 8);
		memset(&iphdr->net_ipdst[8], 0, 8);
		iphdr->net_ipdst[11] = 0xff;
		iphdr->net_ipdst[12] = 0xfe;
		kprintf("_6lpread: cptr inc 10\n");
		iphdr->net_ipdst[14] = *cptr++;
		iphdr->net_ipdst[15] = *cptr++;
		break;
	 case 3:
	 	memcpy(iphdr->net_ipdst, ip_llprefix, 8);
		memcpy(&iphdr->net_ipdst[8], encdstiid, 8);
		break;
	}
	uptr = iphdr->net_ipdata;

	while(uncompress) {

		ipnhc = *cptr++;

		if((ipnhc & LOWPAN_DISPATCH_MASK) != LOWPAN_IPNHC) {
			return SYSERR;
		}

		if(((ipnhc >> LOWPAN_IPNHC_EID) & 0x07) == 7) {
			*prevnh = IP_IPV6;
			iplen = _6lpread(cptr, inlen - (cptr - (byte *)inptr), uptr, &iphdr->net_ipsrc[8], &iphdr->net_ipdst[8]);
			iplen += (uptr - iphdr->net_ipdata);
			iphdr->net_iplen = htons(iplen);
			return 40 + iplen;
		}

		switch((ipnhc >> LOWPAN_IPNHC_EID) & 0x07) {

		 case 0:
		 	*prevnh = IP_HBH;
			if(ipnhc & 0x01) {
				prevnh = uptr++;
			}
			else {
				*uptr++ = *cptr++;
				uncompress = FALSE;
			}
			optlen = *cptr++;
			*uptr++ = (optlen - 6)/8;
			memcpy(uptr, cptr, optlen);
			uptr += optlen;
			cptr += optlen;
			break;
		 case 5:
		 case 6:
		 default:
		 	return SYSERR;
		}
	}

	remain = inlen - (cptr - (byte *)inptr);
	kprintf("_6lpread: remain %d, %d %d\n", remain, inlen, (cptr-(byte*)inptr));
	memcpy(uptr, cptr, remain);
	iplen = remain + (uptr - iphdr->net_ipdata);
	iphdr->net_iplen = htons(iplen);

	return 40 + iplen;
}
