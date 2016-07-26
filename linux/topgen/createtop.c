/********************************************************/
/* createtop.c - A program to create various default	*/
/*		topologies, like a complete graph, star	*/
/*		or linear.				*/
/*							*/
/*    IUsage: createtop <topshape> [options] <n>      	*/
/*							*/
/*							*/
/********************************************************/

#define FILELEN 256
#define NAMLEN	128
#define FAMILY	NAMLEN-3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <unistd.h>

int star_options(int argc, char **argv);
int complete_options(int argc, char **argv);
int linear_options(int argc, char **argv);


int main(int argc, char **argv) {
        char *topshape;
        char *usage = "Usage: createtop <topshape> [options] <n>\n";
        char filename[FILELEN] = "outputtop";
        FILE *fout;
        topshape = argv[1];

        if (argc < 2) {
		fprintf(stderr, usage);
		exit(EXIT_FAILURE);
	}

	if (strcmp(topshape, "star") == 0) {
		star_options(argc, argv);
	}

	else if(strcmp(topshape, "complete") == 0) {
		complete_options(argc, argv);
	}

	else if(strcmp(topshape, "linear") == 0) {
		linear_options(argc, argv);
	}

	else {
		printf("%s", usage);
		exit(1);
	}
	exit(0);
}

/********************************************************/
/* star -	a function to generate an adjacency	*/
/*		list topology file for a star graph	*/
/*							*/
/* Usage: createtop star n [-m mode] [-f family_name]	*/
/*					[-t top_name]	*/
/*							*/
/*		n: number of "spokes" in star + center	*/
/*		mode: i, o, or b			*/
/*			i: links point to center	*/
/*			o: links point away from center	*/
/*			b: links are bidirctional	*/
/*		family_name: family name for nodes.	*/
/*			For example, if the family name	*/
/*			is a, the nodes will be named	*/
/*			a_0, a_1 etc.			*/
/*		top_name: name of the output top file	*/
/********************************************************/

int star_options(int argc, char **argv) {
	char family_name[FAMILY] = "node";
	int n;			/* Number of nodes */
	int i;
	int j;
	char mode = 'b';	/* Mode: default is bidirectional */
	char filename[FILELEN] = "outputtop";
	FILE *fout;
	char *usage = "Usage: createtop star [-m mode] [-f family_name] [-t topname] n\n";
	int opt;

	argv++;
	argc--;

	while ((opt = getopt(argc, argv, "m:f:t:")) != -1) {
		switch (opt) {

			/* Get mode for links */
		case 'm':
			mode = optarg[0];
			break;

			/* Family name */
		case 'f':
			strcpy(family_name, optarg);
			break;

			/* Output topology name */
		case 't':
			strcpy(filename, optarg);
			break;
		default:
			fprintf(stderr, "%s", usage);
			exit(EXIT_FAILURE);
		}
	}

	/* Check for mandatory argument - number of nodes */

	if (optind >= argc) {
		fprintf(stderr, usage);
		exit(EXIT_FAILURE);
	}

	n = atoi(argv[optind]);

	/* Open output file */

	fout = fopen(filename, "w+");

	switch (mode) {
	case 'b':
		/* For central node */

		fprintf(fout, "%s_0:", family_name);

		for (i = 1; i < n; i++) {
			fprintf(fout, " %s_%d", family_name, i);
		}

		fprintf(fout, "\n");

		/* For spokes */

		for (i = 1; i < n; i++) {
			fprintf(fout, "%s_%d: %s_0\n", family_name, i, family_name);
		}
		break;

	case 'i':
		/* For central node */
		/* Nothing printed because it can't reach anyone */

		/* For spokes */

		for (i = 1; i < n; i++) {
			/* Just the central node for all nodes */
			fprintf(fout, "%s_%d: %s_0\n", family_name, i, family_name);
		}
		break;

	case 'o':
		/* For central node */
		/* Can reach everyone */
		fprintf(fout, "%s_0:", family_name);

		for (i = 1; i < n; i++) {
			fprintf(fout, " %s_%d", family_name, i);
		}

		fprintf(fout, "\n");

		/* For spokes */
		/* Nothing, because they can't reach anyone */
		break;
	}

	/* Print output file name */
	printf("Output file: %s\n", filename);
	fclose(fout);
	return 0;

}

/********************************************************/
/* complete - a function to generate an adjacency     	*/
/*		list topology file for a complete graph	*/
/*							*/
/*	Usage: createtop complete [-f family_name]	*/
/*					[-t top_name]	*/
/*							*/
/*		n: number of nodes in complete graph	*/
/*		family_name: family name for nodes.	*/
/*			For example, if the family name	*/
/*			is a, the nodes will be named	*/
/*			a_0, a_1 etc.			*/
/*		top_name: name of the output top file	*/
/********************************************************/

int complete_options(int argc, char **argv){
	char *family_name = "node";
	int n;			/* Number of nodes */
	int i;
	int j;
	char *filename = "outputtop";
	FILE *fout;
	char *usage = "Usage: complete [-f family_name] [-t topname] n\n";
	int opt;

	argc--;
	argv++;

	while ((opt = getopt(argc, argv, "f:t:")) != -1) {
		switch (opt) {

			/* Family name */
		case 'f':
			strcpy(family_name, optarg);
			break;

			/* Output topology name */
		case 't':
			strcpy(filename, optarg);
			break;
		default:
			fprintf(stderr, "%s", usage);
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		fprintf(stderr, usage);
		exit(EXIT_FAILURE);
	}

	n = atoi(argv[optind]);

	fout = fopen(filename, "w+");

	for (i = 0; i < n; i++) {
		fprintf(fout, "%s_%d:", family_name, i);
		for (j = 0; j < n; j++) {
			if (i != j)
	fprintf(fout, " %s_%d", family_name, j);
		}
		fprintf(fout, "\n");
	}
	printf("\nOutput file: %s\n", filename);
	fclose(fout);
	return 0;
}

/********************************************************/
/* linear -	a function to generate an adjacency    	*/
/*		list topology file for a linear graph	*/
/*							*/
/*		Usage: createtop linear n [-s]		*/
/*		        [-f family_name]  [-t top_name]	*/
/*							*/
/*		n: number of nodes in star + center	*/
/*		-s: whether links are symmetric or not	*/
/*							*/
/*		family_name: family name for nodes.	*/
/*			For example, if the family name	*/
/*			is a, the nodes will be named	*/
/*			a_0, a_1 etc.			*/
/*		top_name: name of the output top file	*/
/********************************************************/

int linear_options(int argc, char **argv){
	char family_name[FAMILY] = "node";
	int n;			/* Number of nodes */
	int i;
	int j;
	char filename[FILELEN] = "outputtop";
	FILE *fout;
	char *usage = "Usage: linear [-s] [-f family_name] [-t topname] n\n";
	int opt;
	int symmetric = 0;		/* Symmetry flag */

	argv++;
	argc--;

	while ((opt = getopt(argc, argv, "sf:t:")) != -1) {
		switch (opt) {
		case 's':
			symmetric = 1;
			break;
		case 'f':
			strcpy(family_name, optarg);
			break;
		case 't':
			strcpy(filename, optarg);
			break;
		default: /* '?' */
			fprintf(stderr, "%s", usage);
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		fprintf(stderr, usage);
		exit(EXIT_FAILURE);
	}

	n = atoi(argv[optind]);

	fout = fopen(filename, "w+");

	switch (symmetric) {
	case 0:
		/* Starting with tail end */
		for (i = 0; i < n-1; i++) {
			fprintf(fout, "%s_%d: %s_%d\n", family_name, i, family_name, i+1);
		}
		break;

	case 1:
		/* Starting with one end */
		fprintf(fout, "%s_0: %s_1\n", family_name, family_name);

		for (i = 1; i < n-1; i++) {
			/* Just the central node for all nodes */
			fprintf(fout, "%s_%d: %s_%d %s_%d\n", family_name, i, family_name, i-1, family_name, i+1);
		}

		/* Finish off with the other end */
		fprintf(fout, "%s_%d: %s_%d\n", family_name, i, family_name, i-1);
		break;
	}

	/* Print output file name */
	printf("Output file: %s\n", filename);
	fclose(fout);
	return 0;
}
