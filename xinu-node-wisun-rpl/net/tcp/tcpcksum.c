/* tcpcksum.c  -  cksum, tcpcksum */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  localcksum  -  compute a checksum given an address and length
 *------------------------------------------------------------------------
 */
static	uint32	localcksum(
	  char		*addr,		/* Address of items to checksum	*/
	  int32		len		/* Length of items in bytes	*/
	)
{
	uint16	*hptr;			/* Ptr to 16-bit items		*/ 
	int32	i;			/* Counts 16-bit values		*/
	uint16	word;			/* one 16-bit word		*/
	uint32	sum;			/* computed value of checksum	*/

	hptr= (uint16 *) addr;
	sum = 0;
	len = len >> 1;
	for (i=0; i<len; i++) {
		word = *hptr++;
		sum += (uint32) htons(word);
	}

	return sum;
	/*
	sum += 0xffff & (sum >> 16);
	sum = 0xffff & ~sum;
	if (sum == 0xffff) {
		sum = 0;
	}
	return (uint16) (0xffff & sum);
	*/
}

/*------------------------------------------------------------------------
 *  tcpcksum  -  compute the TCP checksum for a packet with IP header
 *		 in host byte order and TCP in network byte order
 *------------------------------------------------------------------------
 */
uint16	tcpcksum(
	struct netpacket *pkt		/* Pointer to packet	*/
	)
{
	uint32	sum;			/* Computed checksum		*/
	uint16	*ptr;			/* Walks along the segment	*/
	uint16	len;			/* Length of the TCP segment	*/
	int32	i;			/* Counts 16-bit items in IP	*/
					/*   source and dest. addrs.	*/
	struct {			/* Pseudo header		*/
		byte	ipsrc[16];
		byte	ipdst[16];
		uint32	tcplen;
		byte	zeros[3];
		byte	ipnh;
	}	pseudo;

	/* Compute the length of the TCP segment,	*/
	/*   adding a zero byte if the length is odd	*/

	len = pkt->net_iplen;

	/* Form a pseudo header */

	memset(&pseudo, 0, sizeof(pseudo));
	memcpy(pseudo.ipsrc, pkt->net_ipsrc, 16);
	memcpy(pseudo.ipdst, pkt->net_ipdst, 16);
	pseudo.ipnh = IP_TCP;
	pseudo.tcplen  = htons(len);

	/* Adjust the length to an even number for the computation */

	if (len % 2) {
		*( ((char *)&pkt->net_tcpsport) + len ) = NULLCH;
		len++;
	}

	/* Start the checksum at zero */

	sum = 0;

	/* Add in the "pseudo header" values */

	ptr = (uint16 *)&pseudo;
	for (i = 0; i < 20; i++) {
		sum += htons(*ptr);
		ptr++;
	}

	/* Compute the checksum over the TCP segment */

	//sum += ~localcksum ((char *)&pkt->net_tcpsport, len) & 0xffff;
	sum += localcksum ((char *)&pkt->net_tcpsport, len);

	/* Add overflow */

	sum = (sum & 0xffff) + (sum >> 16);

	/* return 16 bits of the value */

	return ~((sum & 0xffff) + (sum >> 16)) & 0xffff;
}
