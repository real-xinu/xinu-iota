/*  main.c  - main */

#include <xinu.h>
#include <stdio.h>

extern uint32 nsaddr;

process	main(void)
{

	net_init();

	struct	netpacket *pkt;

	if(if_tab[0].if_eui64[7] == 129) {
		struct ipinfo ipdata;
		memset(&ipdata, 0, sizeof(ipdata));
		ipdata.iphl = 255;
		memcpy(ipdata.ipsrc, ip_llprefix, 16);
		ipdata.ipsrc[15] = 129;
		memcpy(ipdata.ipdst, ip_llprefix, 16);
		ipdata.ipdst[15] = 130;
		icmp_send(0, 128, 0, &ipdata, "echo", 5);
	}
	else if(if_tab[0].if_eui64[7] == 130) {
		int32 slot = icmp_register(0, ip_unspec, ip_unspec, 128, 0);
		char buf[50];
		struct ipinfo ipdata;
		memset(&ipdata, 0, sizeof(ipdata));
		if(slot != SYSERR) {
			int32 readcount = icmp_recvaddr(slot, buf, 10, 40000, &ipdata);
			if(readcount > 0) {
				kprintf("sender: ");
				int32 i;
				for(i = 0; i < 16; i += 2) {
					kprintf("%04X:", htons(*((uint16 *)&ipdata.ipsrc[i])));
				}
				kprintf("\n");
				kprintf("read: %s\n", buf);
			}
			else {
				kprintf("icmp_recv failed with %d\n", readcount);
			}
		}
		else {
			kprintf("icmp_register failed\n");
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
