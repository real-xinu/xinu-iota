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

	struct etherPkt *pkt = getmem(1514);
	memset(pkt->dst, 0xff, 6);
	memcpy(pkt->src, ethertab[0].devAddress, 6);
	pkt->type = 0x1234;

	int i;
	for(i = 0; i  <10; i++) {
		write(ETHER0, pkt, 100);
		sleep(1);
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
