/* mkaddr.c  -  mkaddr */

/************************************************************************/
/*									*/
/* mkaddr - initialize an Ethernet multicast address to all 0s except	*/
/*		for the multicast bit.  (srbit is used to set bits	*/
/*		that correspond to individual nodes.)			*/
/*									*/
/************************************************************************/
#include "include/compatibility.h"
void	mkaddr (
    byte	addr[]		/* Array to hold a 48-bit Ethernet	*/
    /*	multicast address		*/
)
{
    addr[0] = 0x01;
    addr[1] = addr[2] = addr[3] = addr[4] = addr[5] = 0x00;
    return;
}



/* srbit.c  -  srbit */

#define	BIT_SET		0	/* Command to set a bit  */
#define	BIT_TEST	1	/* Command to test a bit */

/************************************************************************/
/*									*/
/* srbit - set or read the bit in a 48-bit Ehternet multicast address	*/
/*		that corresponds to a node ID.				*/
/*									*/
/* Arguments are:							*/
/*									*/
/*	The node ID in the range 0 through 45				*/
/*	Pointer to a multicast address (array of six bytes)		*/
/*	operation: either BIT_SET or BIT_TEST				*/
/*									*/
/* Bit assignment:							*/
/*	A multicast address is placed in an array of bytes in memory,	*/
/*	stored in big-endian byte order.  Each node ID corresponds to	*/
/*	one of the bits, starting at the least-significant bit in the	*/
/*	least-significant byte.  Thus, to set the bit for node 0,	*/
/*	we compute the logical or of mask 0x01 with address[5].  Node 1	*/
/*	uses 0x02 with address[5].  Node 7 uses mask 0x80 with		*/
/*	address[5], and node 8 uses mask 0x01 with address[4].  The	*/
/*	only exception occurs in the high-order byte (address[0])	*/
/*	because Ethernet reserves the low-order bit for multicast.	*/
/*	Therefore, the assignment of bits in high-order byte is		*/
/*	shifted one bit to the left (i.e., the first bit we use is	*/
/*	0x02.								*/
/*									*/
/************************************************************************/
int	srbit (
    byte	addr[],		/* A 48-bit Ethernet multicast address	*/
    int	nodeid,		/* A node ID (0 - 45)			*/
    int	cmd		/* BIT_SET or BIT_TEST			*/
)
{
    int	aindex;		/* The byte index in the address array	*/
    /*	(0 through 5)			*/
    int	mask;		/* A byte with a 1 in the bit position	*/

    /*	to use and 0 in other positions	*/
    /* Ensure that the bit value is valid */
    if ( (nodeid < 0) || (nodeid > 45) ) {
        return SYSERR;
    }

    /* Compute a mask 2^(floor(nodeid modulo 8))*/
    mask = 1 << (nodeid % 8);
    /* Compute the appropriate array index */
    aindex = 5 - (nodeid >> 3);

    /* Adjust the mask one bit for the high-order byte */
    if (aindex == 0) {
        mask = mask << 1;
    }

    /*DEBUG*/ //printf("Bit number %2d is in address[%d] and the mask is 0x%02x",

    /*DEBUG*///			nodeid, aindex, mask);

    /* If command specifies setting, change set the bit */
    if (cmd == BIT_SET) {
        addr[aindex] |= mask;
        return 1;
    }

    /* Command specifies testing */
    if ( (addr[aindex] & mask) == 0) {
        return 0;	/* Bit is 0 */

    } else {
        return 1;	/* Bit is 1 */
    }
}
