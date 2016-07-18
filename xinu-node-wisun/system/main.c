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
extern	byte ip_llprefix[];

process	main(void)
{
	/* Start the network */
	net_init();
	
	
        sleep(3);

	kprintf("\n...creating a node\n");
	recvclr();


       
	//resume(create(shell, 8192, 50, "shell", 1, CONSOLE));

	/* Wait for shell to exit and recreate it */
        wsnode_join();
	sleep(1);
	//init_udp_monitor();
        resume(create(wsnodeapp, 8192, 50, "wsnodeapp", 1, CONSOLE));
        
	


	/*while (TRUE) {
		receive();
		sleepms(200);
		kprintf("\n\nMain process recreating shell\n\n");
		resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
	}*/
	return OK;
}
