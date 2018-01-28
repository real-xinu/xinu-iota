/* xsh_ps.c - xsh_ps */

#include <xinu.h>
#include <stdio.h>
#include <string.h>

/*------------------------------------------------------------------------
 * xsh_ps - shell command to print the process table
 *------------------------------------------------------------------------
 */
shellcmd plossdump(int nargs, char *args[])
{
	if (nargs != 1) {
        kprintf("No args required\n");
        return 0;
    }
    
    kprintf("a_msg: %d\n", sizeof(struct a_msg));
    kprintf("linkinfo: %d\n", sizeof(info.link_info[0]));

    kprintf("Node   Threshold   PlossRef    PlossExp    DistRef     Dist    Sigma\n");
    int i;

    for (i = 0; i < 46; i++) {
        kprintf("%d: %d, %d, %d, %d, %d, %d\n", i, info.link_info[i].threshold,
info.link_info[i].pathloss_ref, info.link_info[i].pathloss_exp, info.link_info[i].dist_ref,
info.link_info[i].distance, info.link_info[i].sigma);
    }

	return 0;
}
