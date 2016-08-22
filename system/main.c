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
	//net_init();
	//tcp_init();

	int32	tslot = tcp_register(iftab[0].if_ipucast[0].ipaddr, 50001, 0, 0);
	kprintf("TCP slot = %d\n", tslot);

	int32	nslot;
	int32	rv;

	rv = tcp_recv(tslot, &nslot, 4);
	kprintf("TCP connected: %d\n", nslot);

	if(rv != SYSERR) {
		while(TRUE) {
			tcp_send(nslot, "Ping...\r\n", 10);
			sleep(1);
		}
	}

	kprintf("\n...creating a shell\n");
	recvclr();
	resume(create(shell, 8192, 50, "shell", 1, CONSOLE));

	/* Wait for shell to exit and recreate it */

	while (TRUE) {
		receive();
		sleepms(200);
		kprintf("\n\nMain process recreating shell\n\n");
		resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
	}

	return OK;
}
