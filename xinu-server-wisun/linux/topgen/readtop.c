/* readtop.c - read a topology database file and generate a human     	*/
/*	readable version of the topology				*/

/************************************************************************/
/*									*/
/* use:   readtop top_database_name					*/
/*									*/
/* readtop takes a binary topology database file as input, and prints	*/
/* the reachability information for each node in a human readable	*/
/* format. For example, if a node 'a' can reach nodes 'b' and 'c',	*/
/* the entry for 'a' will read						*/
/*									*/
/*			a: b c				       		*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NODES 46
#define NAMLEN 128

struct	node {			/* An entry in the list of node names	*/
	int	nstatus;	/* Have we seen this node already?	*/
	int	nrecv;		/* Can the node receive from any other?	*/
	int	nsend;		/* Can the node send to any others?	*/
	char	nname[NAMLEN];	/* Name of the node			*/
	unsigned char nmcast[6];/* Multicast address to use when sending*/
};

struct node nodes[NODES];
int nnodes = 0;

#define	BIT_SET		 0	/* Command to set a bit   */
#define	BIT_TEST	 1	/* Command to test a bit  */
#define BIT_RESET	 2	/* Command to reset a bit */
/* Definitions to make code compatibile with Xinu */
#define	SYSERR		-1
typedef	unsigned char	byte;

int	srbit (
	  byte	addr[],		/* A 48-bit Ethernet multicast address	*/
	  int	nodeid,		/* A node ID (0 - 45)			*/
	  int	cmd		/* BIT_SET or BIT_TEST or BIT_RESET   	*/
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

	/* If command specifies setting, change set the bit */

	if (cmd == BIT_SET) {
		addr[aindex] |= mask;
		return 1;
	}

	/* If command specifies resetting, reset the bit to 0 */

	if (cmd == BIT_RESET) {
		addr[aindex] &= ~mask;
		return 2;
	}

	/* Command specifies testing */

	if ( (addr[aindex] & mask) == 0) {
		return 0;	/* Bit is 0 */
	} else {
		return 1;	/* Bit is 1 */
	}
}


int main(int argc, char **argv) {
	int i, j;
	FILE *fin;
	unsigned char buffer[6];
	unsigned char namebuffer[NAMLEN];
	unsigned char sentinel[6];
	unsigned char length[1];
	unsigned char name[NAMLEN];
	char *use = "usage: readtop top_database_name\n";

	struct node *nptr;

	if (argc < 2) {
	        printf("%s", use);
		exit(1);
	}

	char *infile = argv[1];
	sentinel[0] = sentinel[1] = sentinel[2] = sentinel[3] =
			sentinel[4] = sentinel[5] = 0x00;

	fin = fopen(infile, "rb");
	if (fin == NULL) {
		printf("Can't open file");
		exit(1);
	}

	while(fread(buffer, 1, sizeof(buffer), fin) > 0) {
      		if(memcmp(buffer, sentinel, 6) == 0) {
			break;
		}
		nptr = &nodes[nnodes++];
		for(int j = 0; j < 6; j++) {
			nptr -> nmcast[j] = buffer[j];
		}
	}

	/* Read node names */

	nnodes = 0;
	while(fread(buffer, 1, 1, fin) > 0) {
		memcpy(length, buffer, 1);
		fread(namebuffer, 1, (int) buffer[0], fin);
		memcpy(name, namebuffer, (int)buffer[0]);
		nptr = &nodes[nnodes++];
		strcpy(nptr->nname, name);
	}

	fclose(fin);

	for (i = 0; i < nnodes; i++) {
		printf("%s: ", nodes[i].nname);
		for (j = 0; j < nnodes; j++) {
			if(srbit(nodes[i].nmcast, j, BIT_TEST)) {
				printf("%s ", nodes[j].nname);
			}
		}
		printf("\n");
	}
	return 0;
}
