/* xsh_link.c - xsh_link */

#include <xinu.h>
#include <stdio.h>
extern	byte adj[96][96];
/*------------------------------------------------------------------------
 * xsh_link  -  Add or remove a link in Adjacency matrix
 *------------------------------------------------------------------------
 */
shellcmd xsh_link(int nargs, char *args[])
{
	int32	node1, node2;
	intmask	mask;

	if(nargs != 4) {
		fprintf(stderr, "Incorrect number of arguments\n");
		fprintf(stderr, "Usage: link <add/del/status> <node1> <node2>\n");
		return 1;
	}

	node1 = atoi(args[2]);
	node2 = atoi(args[3]);

	if( (node1 < 101) || (node1 > 196) ||
	    (node2 < 101) || (node2 > 196) ) {
		fprintf(stderr, "Invalid node number. Allowed range [101,196]\n");
		return 1;
	}

	mask = disable();
	if(!strcmp(args[1], "add")) {
		adj[node1-101][node2-101] = 1;
	}
	else if(!strcmp(args[1], "del")) {
		adj[node1-101][node2-101] = 0;
		adj[node2-101][node1-101] = 0;
	}
	else if(!strcmp(args[1], "status")) {
		printf("Node %d and Node %d are ", node1, node2);
		if(adj[node1-101][node2-101]) {
			printf("connected\n");
		}
		else {
			printf("not connected\n");
		}
	}
	else {
		fprintf(stderr, "Invalid argument %s to link\n", args[1]);
		fprintf(stderr, "Usage: link <add/del/status> <node1> <node2>\n");
	}

	restore(mask);
	return 0;
}
