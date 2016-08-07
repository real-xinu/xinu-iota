#include "../include/headers.h"
#include "../include/prototypes.h"
#include "../include/macros.h"
#include "../include/global.h"

FILE *fp;
FILE *log_f;
int s;
struct sockaddr_in si_other;
char map_list[46][100];

char *log_file_path = "../log/log.txt";

/* --------------------------
 * Main- Call UDP process
 * -------------------------*/
int main (int argc, char **argv)
{
    struct timeval  tv1, tv2;
    gettimeofday (&tv1, NULL);
    char  use[] = "error: use is tbed ( IP | <script> -f | <script> -s )\n";
    log_f = fopen (log_file_path, "w");

    if ( (argc != 2)  && (argc != 3) ) {
        fprintf (stderr, "%s", use);
        exit (1);
    }

    if (argv[2] != NULL) {
        if (!strcmp ("-s", argv[2])) {
            fp = stdout;

        } else if (!strcmp ("-f", argv[2] )) {
            fp = fopen (make_result_file (argv[1]), "w");
        }

        SRV_IP = "0.0.0.0";
        udp_process (SRV_IP, argv[1]);

    } else {
        fp = stdout;
        SRV_IP = argv[1];
        udp_process (SRV_IP, argv[2]);
    }

    gettimeofday (&tv2, NULL);
    fprintf (fp, "Emulation time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));
    fclose (log_f);
    return 0;
}
