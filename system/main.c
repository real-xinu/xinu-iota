/*  main.c  - main */

#include <xinu.h>
#include <stdio.h>

extern uint32 nsaddr;

process	main(void)
{
	net_init();

	byte ula[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	byte all_rtr[16] = {0};
	all_rtr[0] = 0xff;
	all_rtr[1] = 0x02;
	all_rtr[15] = 0x02;
#if 0
	struct	netpacket *pkt;

	char cmd[5];
	kprintf("Enter command [s or c]: ");
	read(CONSOLE, cmd, 5);
	if(cmd[0] == 's') {
	//if(if_tab[0].if_hwucast[7] == 111) {
		int32	slot = tcp_register(0, ip_unspec, 12345, 0);
		if(slot == SYSERR) {
			panic("Cannot register tcp slot\n");
		}
		int32	new;
		int32	rv = tcp_recv(slot, &new, 4);
		kprintf("New connection: %d\n", new);

		tcp_send(new, "hello world!!!!", 16);
		tcp_close(new);
		while(1);
	}
	else {
	//if(if_tab[0].if_hwucast[7] == 112) {
		//xsh_ps(0, NULL);
		byte	remip[16];
		memcpy(remip, ip_llprefix, 16);
		byte	server[5];
		kprintf("Enter server's backend number: ");
		read(CONSOLE, &server, 5);
		server[3] = 0;
		remip[15] = atoi(server);
		//remip[15] = 111;
		int32	slot = tcp_register(0, remip, 12345, 1);
		kprintf("main: -------------------------------------------registered %d\n", slot);
		if(slot == SYSERR) {
			panic("Cannot connect to server\n");
		}
		char buf[10] = {0};
		kprintf("------------------------------main: calling tcp_recv\n");
		int32	rv = tcp_recv(slot, buf, 10);
		kprintf("Received from server: %s\n", buf);
		tcp_close(slot);
		while(1);
	}
	if(if_tab[0].if_hwucast[7] == 111) {
		if_tab[0].if_ndData.sendadv = TRUE;
		memcpy(if_tab[0].if_ipucast[1].ipaddr, ula, 8);
		memcpy(&if_tab[0].if_ipucast[1].ipaddr[8], if_tab[0].if_hwucast, 8);
		if_tab[0].if_nipucast++;
		memcpy(if_tab[0].if_ipmcast[0].ipaddr, all_rtr, 16);
		if_tab[0].if_nipmcast = 1;
		char buf;
		read(CONSOLE, &buf, 1);
		kprintf("sending nbr sol to 102\n");
		struct ipinfo ipdata;
		memcpy(ipdata.ipsrc, if_tab[0].if_ipucast[0].ipaddr, 16);
		memcpy(ipdata.ipdst, ula, 8);
		memset(&ipdata.ipdst[8], 0, 8);
		ipdata.ipdst[15] = 112;
		ipdata.iphl = 255;
		struct nd_nbrsol nbrsol;
		kprintf("calling icmp_send now\n");
		icmp_send(0, ICMP_TYPE_NBRSOL, 0, &ipdata, (char *)&nbrsol, sizeof(nbrsol));
	}
	if(if_tab[0].if_hwucast[7] == 112) {
		struct ipinfo ipdata;
		memcpy(ipdata.ipsrc, ip_llprefix, 16);
		ipdata.ipsrc[15] = 112;
		memcpy(ipdata.ipdst, ip_llprefix, 16);
		ipdata.ipdst[15] = 111;
		ipdata.iphl = 255;
		/*byte *buf = getmem(1000);
		struct nd_rtradv *rtradv = buf;
		rtradv->currhl = 255;
		struct nd_pi *pio = rtradv->opts;
		pio->type = ND_TYPE_PIO;
		pio->len = 4;
		pio->preflen = 64;
		pio->a = 1;
		memset(pio->prefix, 0, 16);
		memcpy(pio->prefix, ula, 8);
		icmp_send(0, ICMP_TYPE_RTRADV, 0, &ipdata, buf, sizeof(struct nd_rtradv)+sizeof(struct nd_pi));
		sleep(2);
		memcpy(ipdata.ipdst, ula, 8);
		icmp_send(0, 1, 0, &ipdata, "hello", 5);*/
		struct nd_rtrsol rtrsol;
		memset(&rtrsol, 0, sizeof(rtrsol));
		memcpy(ipdata.ipdst, all_rtr, 16);
		icmp_send(0, ICMP_TYPE_RTRSOL, 0, &ipdata, &rtrsol, sizeof(rtrsol));
		memcpy(ipdata.ipdst, if_tab[0].if_ipucast[0].ipaddr, 16);
		ipdata.ipdst[15] = 111;
		sleep(2);
		nd_reg_address(0, 1, ipdata.ipdst);
	}
/*
	if(if_tab[0].if_hwucast[7] == 101) {
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
*/
#endif
	struct	ipinfo ipdata;
	char	buf[50] = {0};
	byte	ipdst[] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0,
				0x2, 0x24, 0xe8, 0xff, 0xfe, 0x3a, 0xb0, 0x73};
	memcpy(ipdata.ipdst, ipdst, 16);
	memcpy(ipdata.ipsrc, if_tab[1].if_ipucast[0].ipaddr, 16);
	ipdata.iphl = 255;

	kprintf("Sending packet to ");
	ip_print(ipdata.ipdst);
	kprintf("\n");
	icmp_send(1, 128, 0, &ipdata, buf, 50);
	sleep(50);
	icmp_send(1, 128, 0, &ipdata, buf, 50);

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
