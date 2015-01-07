/*  main.c  - main */

#include <xinu.h>
#include <stdio.h>

extern uint32 nsaddr;

process	main(void)
{

	net_init();

	struct	netpacket *pkt;

	if(if_tab[0].if_eui64[7] == 101) {
		int32 slot[2];
		int32	retval;
		slot[0] = icmp_register(0, ip_unspec, ip_unspec, 1, 0);
		slot[1] = icmp_register(0, ip_unspec, ip_unspec, 2, 0);
		kprintf("slots: %d %d\n", slot[0], slot[1]);

		char buf[100];
		uint32 len=100;

		while(1) {
			retval = icmp_recvn(slot, 2, buf, &len, 200000);
			kprintf("Recvd %d bytes from slot %d\n", len, retval);
		}
	}
	else {
		struct ipinfo ipdata;
		memcpy(ipdata.ipsrc, ip_llprefix, 16);
		ipdata.ipsrc[15] = 102;
		memcpy(ipdata.ipdst, ip_llprefix, 16);
		ipdata.ipdst[15] = 101;
		ipdata.iphl = 255;

		icmp_send(0, 1, 0, &ipdata, "hello", 5);
		sleep(3);
		icmp_send(0, 2, 0, &ipdata, "world", 5);
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
