#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*---------------------------------------------------------
 * ISNUMERIC Function: To check a string is number or not
 * -------------------------------------------------------*/
int isnumeric (char *str)
{
    int len = strlen (str);
    int i;

    for (i = 0; i < len; i++) {
        if ((int)str[i] < 48 || (int)str[i] > 57)
            return 0;
    }

    return 1;
}
