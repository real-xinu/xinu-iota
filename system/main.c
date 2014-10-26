/*  main.c  - main */

#include <xinu.h>
#include <stdio.h>

extern uint32 nsaddr;

process	main(void)
{
	/* Start the network */

	//netstart();
	net_init();

	//nsaddr = 0x800a0c10;

	/*if(if_tab[0].if_eui64[7] == 121) {
		while(TRUE) {
			char c;
			struct netpacket *pkt = getmem(PACKLEN);
			memset(pkt, NULLCH, PACKLEN);
			pkt->net_pandst[7] = 122;
			write(ETHER0, pkt, 200);
			read(CONSOLE, &c, 1);
		}
	}*/

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
