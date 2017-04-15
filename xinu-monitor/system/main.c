/*  main.c  - main */

#include <xinu.h>

process	main(void)
{
	sleep(3);

	struct	netpacket *pkt;

	pkt = (struct netpacket *)getbuf(netbufpool);

	memcpy(pkt->net_ethsrc, NetData.ethucast, 6);
	memset(pkt->net_ethdst, 0xff, 6);
	pkt->net_ethtype = htons(ETH_TYPE_A);
	pkt->amsg.amsgtyp = htonl(A_JOIN_MONITOR);
	pkt->amsg.anodeid = 0;

	kprintf("Sending Monitor Join message\n");
	write(ETHER0, (char *)pkt, 100);

	/* Run the Xinu shell */

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
