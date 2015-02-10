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

	struct	netpacket *pkt;

	if(if_tab[0].if_eui64[7] == 111) {
		if_tab[0].if_ndData.sendadv = TRUE;
		memcpy(if_tab[0].if_ipucast[1].ipaddr, ula, 8);
		memcpy(&if_tab[0].if_ipucast[1].ipaddr[8], if_tab[0].if_eui64, 8);
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
	if(if_tab[0].if_eui64[7] == 112) {
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
*/
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
