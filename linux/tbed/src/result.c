#include "../include/headers.h"
#include "../include/prototypes.h"
#include "../include/macros.h"
#include "../include/global.h"


/*--------------------------------
 * Make result file in result directory
 * -------------------------------*/
char * make_result_file (char * script)
{
    int i, ind;
    int k = 0;

    printf("%s\n", script);

    for (i = 0; i < strlen (script); i++) {
        if (script[i] == '/')
            ind = i;
    }

    char * in_file = (char *) malloc (1 + strlen (script) - ind);

    printf("%d\n",ind);
    for (i = ind + 1; i < strlen (script); i++) {
        in_file[k] = script[i];
        k++;
    }

    printf("%s\n",in_file);
    char *result = (char *) malloc (1 + strlen (RES_PATH) + strlen (in_file));
    strcpy (result, RES_PATH);
    strcat (result, in_file);
    printf("result file: %s\n", result);
    return result;
}



