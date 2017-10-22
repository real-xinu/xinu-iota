/*  main.c  - main */

#include <xinu.h>

process	main(void)
{
	/* Obtain an IP address */

	netstart();

	/* Run the Xinu shell */


	//resume(create(shell, 8192, 50, "shell", 1, CONSOLE));

	/* Wait for shell to exit and recreate it */

        kprintf("\n...creating testbed server process\n");
	recvclr();
	init_topo();
	resume(create(wsserver, 8192, NETPRIO-2, "wsserver", 1, CONSOLE));
	//init_topo();
	/*while (TRUE) {
		receive();
		sleepms(200);
		kprintf("\n\nMain process recreating shell\n\n");
		resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
	}*/
	return OK;
    
}
