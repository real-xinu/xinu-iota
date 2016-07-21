#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern const char * const  cmdtab[];
extern int ncmd;
void help()
{
    int maxlen = 0;
    //int ncmd = sizeof(cmdtab)/ sizeof(const char *);
    int i = 0, j;
    int len, cols, spac, lines;

    for (i = 0; i < ncmd ; i++) {
        len = strlen (cmdtab[i]) ;

        if (len > maxlen) {
            maxlen = len;
        }
    }

    cols = 80 / (maxlen + 1);

    if (cols > 6) {
        cols = 6;
    }

    spac = 80 / cols;
    lines = (ncmd + (cols - 1)) / cols;

    for (i = 0; i < lines; i++) {
        for (j = i ; j < ncmd; j += lines) {
            len = strlen (cmdtab[j]);
            printf ("%s", cmdtab[j]);

            while (len < spac) {
                printf (" ");
                len++;
            }
        }

        printf ("\n");
    }
}
