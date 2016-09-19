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

	wsnode_join();

	byte	prefix[] = {0x20, 0x01, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0};

	memcpy(iftab[1].if_ipucast[1].ipaddr, prefix, 8);
	memcpy(iftab[1].if_ipucast[1].ipaddr+8, iftab[1].if_ipucast[0].ipaddr+8, 8);

	memcpy(iftab[1].if_ipmcast[iftab[1].if_nipmcast].ipaddr, ip_allrplnodesmc, 16);
	iftab[1].if_nipmcast++;

	while(1) {
		char	c;
		kprintf("Enter r for root, or n for non-root:");
		read(CONSOLE, &c, 1);
		if(c == 'r') {
			kprintf("Running as root\n");

			iftab[1].if_nipucast++;

			rpl_lbr_init();

			break;
		}
		else if(c == 'n') {
			kprintf("Running as a node\n");
			rpl_send_dis(-1, 1);
			break;
		}
	}

	while(1);


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
