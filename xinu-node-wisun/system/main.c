/*  main.c  - main */

#include <xinu.h>
#include <stdio.h>
int32 cpudelay;
volatile uint32	gcounter = 400000000;
process	counterproc() {

	while(gcounter > 0) {
		gcounter--;
	}
	return OK;
}

process	main(void)
{
	/* Start the network */
	net_init();

	sleep(7);

	kprintf("\n...creating a node\n");
	recvclr();
	//resume(create(shell, 8192, 50, "shell", 1, CONSOLE));

	wsnode_join();
        sleep(2);

        //resume(create(udp_app, 8192, 50, "udp", 1, CONSOLE));
	
	/* Wait for shell to exit and recreate it */

	/*
	while (TRUE) {
		receive();
		sleepms(200);
		kprintf("\n\nMain process recreating shell\n\n");
		resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
	}
	*/
	return OK;
}
