#include "../include/headers.h"
#include "../include/macros.h"
#include "../include/global.h"

char map_list[46][20];
char *path = "../../../remote-file-server/";
int nnodes = 0;
/*--------------------------------------------------------------
 * Make a mapping list between nodes' name and nodes' ID
 * ------------------------------------------------------------*/
void mapping_list (char *fname)
{
    int i;
    //char name[100];
    nnodes = 0;
    unsigned char name_size[1];
    char *file_name = (char *) malloc (1 + strlen (path) + strlen (fname));
    strcpy (file_name, path);
    strcat (file_name, fname);
    FILE *fp;
    unsigned char nmcast[6];
    int zeros = 0;
    fp = fopen (file_name, "rb");

    if (fp == NULL) {
        printf ("Cannot open the file. \n");
        printf("Please make sure the file is in the remote file server dir\n");
        return;
    }

    memset (map_list, 0, sizeof (map_list));

    while (fread (nmcast, sizeof (nmcast), 1, fp) > 0) {
        for (i = 0; i < 6; i++) {
            if ((int)nmcast[i] == 0)
                zeros++;
        }

        if (zeros == 6) {
            break;

        } else
            nnodes++;

        zeros = 0;
    }

    /* DEBUG */ //printf("number of nodes: %d\n",nnodes);
    i = 0;

    while (i < nnodes) {
        fread (name_size, sizeof (name_size), 1, fp);
        fread (map_list[i], (int) *name_size, 1, fp);
        //printf("name is: %s:%d\n" ,map_list[i], (int)*name_size);
        i++;
    }
}


