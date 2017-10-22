/* xsh_rpl.c */

#include <xinu.h>

/*------------------------------------------------------------------------
 * xsh_rpl - shell command to print RPl info
 *------------------------------------------------------------------------
 */
shellcmd xsh_rpl(int nargs, char *args[]) {

	struct	rplentry *rplptr;
	int32	pparent;
	int32	i, j;
	uint32	t, tmax;
	char	ipstr[100];
	char	*pargs[2];
	uint16	*ptr16;
	int32	cursor;
	int32	total;

	rplptr = &rpltab[0];

	if(rplptr->root) {

		tmax = 0;
		total = 0;
		kprintf("sizes %d %d %d\n", NRPLNODES, NBRTAB_SIZE, ND_NCACHE_SIZE);
		for(i = 1; i < NRPLNODES; i++) {
			if(rplnodes[i].state == RPLNODE_STATE_FREE) {
				continue;
			}

			pparent = rplnodes[i].prefparent;
			ip_printaddr(rplnodes[i].ipprefix);
			kprintf(" :: ");
			ip_printaddr(rplnodes[pparent].ipprefix);
			t = rplnodes[i].time - info.assign_times[info.ntimes-1];
			if(tmax < t) {
				tmax = t;
			}
			kprintf(", Time: %d, Max seq: %d\n", rplnodes[i].time, rplnodes[i].maxseq);
			total++;
		}
		kprintf("Total nodes: %d, Setup Time: %d\n", total, tmax);

		if(nargs > 2) {
			for(i = 1; i < NRPLNODES; i++) {
				if(rplnodes[i].state == RPLNODE_STATE_FREE) {
					continue;
				}

				ptr16 = (uint16 *)rplnodes[i].ipprefix;
				cursor = 0;

				for(j = 0; j < 8; j++) {
					sprintf(ipstr+cursor, "%04X:", htons(*ptr16));
					ptr16++;
					cursor += 5;
				}
				ipstr[cursor-1] = '\0';

				pargs[0] = "ping";
				pargs[1] = ipstr;
				xsh_ping(2, pargs);
			}
		}
	}
	else {
		kprintf("Pref. Parent is: ");
		ip_printaddr(rplptr->neighbors[rplptr->prefparent].lluip);
		kprintf("\n");
	}

	return 1;
}
