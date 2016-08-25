/********************************************************/
/* complete.c - a program to generate an adjacency      */
/*		list topology file for a complete graph	*/
/*							*/
/*	use: complete n [family_name] [top_name]	*/
/*							*/
/*		n: number of nodes in complete graph	*/
/*		family_name: family name for nodes.	*/
/*			For example, if the family name	*/
/*			is a, the nodes will be named	*/
/*			a_0, a_1 etc.			*/
/*		top_name: name of the output top file	*/
/********************************************************/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  char *family_name = "node";
  int n;			/* Number of nodes */
  int i;
  int j;
  char *filename = "outputtop";
  FILE *fout;
  char *usage = "usage: complete n [family_name] [topname]\n";

  if(argc < 2 || argc >= 5) {
    printf("%s", usage);
    exit(1);
  }

  if(argc >= 2) {
    n = atoi(argv[1]);
    if(argc >= 3) {
      family_name = argv[2];
      if(argc >= 4) {
	filename = argv[3];
      }
    }
  }

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
