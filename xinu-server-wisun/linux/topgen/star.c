/********************************************************/
/* star.c -	a program to generate an adjacency      */
/*		list topology file for a star graph	*/
/*							*/
/* use: star n [-m mode] [-f family_name] [-t top_name]	*/
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

#define FILELEN 256
#define NAMLEN	128
#define FAMILY	NAMLEN-3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <unistd.h>

int main(int argc, char **argv) {
  char family_name[FAMILY] = "node";
  int n;			/* Number of nodes */
  int i;
  int j;
  char mode = 'b';		/* Mode: default is bidirectional */
  char filename[FILELEN] = "outputtop";
  FILE *fout;
  char *usage = "usage: star [-m mode] [-f family_name] [-t topname] n\n";
  int opt;

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
