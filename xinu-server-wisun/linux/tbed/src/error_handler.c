#include "../include/headers.h"
#include "../include/prototypes.h"
#include "../include/macros.h"
#include "../include/global.h"


/*--------------------------------------------------------
This function is used for handling errors
------------------------------------------------------*/

void error_handler (char *s)
{
    perror (s);
    exit (1);
}


