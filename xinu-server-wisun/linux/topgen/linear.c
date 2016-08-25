/********************************************************/
/* linear.c -	a program to generate an adjacency      */
/*		list topology file for a linear graph	*/
/*							*/
/*    use: linear n [-s] [-f family_name] [-t top_name]	*/
/*							*/
/*		n: number of nodes in star + center	*/
/*		-s: whether links are symmetric or not 	*/
/*							*/
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
  char filename[FILELEN] = "outputtop";
  FILE *fout;
  char *usage = "usage: linear [-s] [-f family_name] [-t topname] n\n";
  int opt;
  int sflag = 0;		/* Symmetry flag */

  while ((opt = getopt(argc, argv, "sf:t:")) != -1) {
    switch (opt) {
    case 's':
      sflag = 1;
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
  /*   printf("name argument = %s\n", argv[optind]); */

  /* if(argc < 2 || argc >= 5) { */
  /*   printf("%s", usage); */
  /*   exit(1); */
  /* } */

  /* if(argc >= 2) { */
  /*   n = atoi(argv[1]); */
  /*   if(argc >= 3) { */
  /*     family_name = argv[2]; */
  /*     if(argc >= 4) { */
  /* 	filename = argv[3]; */
  /*     } */
  /*   } */
  /* } */

  //printf("%s %s %s %d\n", mode, family_name, filename, n);

  fout = fopen(filename, "w+");

  switch (sflag) {
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
