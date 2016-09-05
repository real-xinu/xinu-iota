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
extern	byte ip_solmc[];

process	main(void)
{
	/* Start the network */
	net_init();

	sleep(5);
	kprintf("\n...creating a shell\n");
	recvclr();
	resume(create(shell, 8192, 50, "shell", 1, CONSOLE));

	if(iftab[IF_TYPE_ETH].if_ipucast[0].ipaddr[15] == 0xf2)
	{
		kprintf("Sending ND NS for ncindex : %d\n", 1);
		int i = 0;
		kprintf("CACHE SIZE: %d\n", ND_NCACHE_SIZE);
		struct nd_ncentry * ncptr;
		for(i = 0; i < ND_NCACHE_SIZE; i++)
		{
			ncptr = &nd_ncache[i];
			if(ncptr->nc_state == NC_STATE_FREE)
				kprintf("%d: FREE\n", i);
		}
	}

	/* Wait for shell to exit and recreate it */

	return OK;
}

/* process test_nd_rad()
{
	kprintf("\ntest_nd_eth\n");
	struct ipinfo ipdata;

	kprintf("IP6: ");
	print_ip(if_tab[IF_RADIO].if_ipucast[0].ipaddr);

	kprintf("HWA: ");
	int32 i = 0;
	for(i = 0; i < 7; i++)
		kprintf("%02x:", if_tab[IF_RADIO].if_hwucast[i]);
	kprintf("%02x\n", if_tab[IF_RADIO].if_hwucast[7]);

	memcpy(ipdata.ipsrc, if_tab[IF_RADIO].if_hwucast, 16);
	kprintf("SRC: ");
	print_ip(ipdata.ipsrc);

	memcpy(ipdata.ipdst, ip_solmc, 16);
	ipdata.ipdst[13] = 0xbb;
	ipdata.ipdst[14] = 0x4a;
	ipdata.ipdst[15] = 0x27;
	memcpy(ipdata.ipdst, if_tab[IF_RADIO].if_ipucast[0].ipaddr, 16);
	kprintf("DST: ");
	print_ip(ipdata.ipdst);
	ipdata.iphl = 255;

	struct nd_nbrsol nd_msg;
	nd_msg.res = 0;
	memcpy(nd_msg.tgtaddr, ip_llprefix, 16);
	nd_msg.tgtaddr[8] = 0x6e;
	nd_msg.tgtaddr[9] = 0xec;
	nd_msg.tgtaddr[10] = 0xeb;
	nd_msg.tgtaddr[11] = 0xff;
	nd_msg.tgtaddr[12] = 0xfe;
	nd_msg.tgtaddr[13] = 0xbb;
	nd_msg.tgtaddr[14] = 0x4a;
	nd_msg.tgtaddr[15] = 0x27;
	kprintf("TGT: ");
	print_ip(nd_msg.tgtaddr);

	int32 res = icmp_send(IF_RADIO, ICMP_TYPE_NBRSOL, 0, &ipdata, (char *)&nd_msg, sizeof(struct nd_nbrsol));
	if(res == SYSERR)
		kprintf("SYSERR\n");
	else
		kprintf("HERE\n");
}

process test_nd_eth()
{
	kprintf("\ntest_nd_eth\n");
	struct ipinfo ipdata;

	kprintf("IP6: ");
	print_ip(if_tab[IF_ETH].if_ipucast[0].ipaddr);

	memcpy(ipdata.ipsrc, if_tab[IF_ETH].if_hwucast, 16);
	kprintf("SRC: ");
	print_ip(ipdata.ipsrc);

	/* Beagle 102 Solicitated Node Multicast Address 
	kprintf("SNM: ");
	print_ip(if_tab[IF_ETH].if_ipmcast[0].ipaddr);

	memcpy(ipdata.ipdst, ip_solmc, 16);
	ipdata.ipdst[13] = 0xbb;
	ipdata.ipdst[14] = 0x4a;
	ipdata.ipdst[15] = 0x27;
	kprintf("DST: ");
	print_ip(ipdata.ipdst);
	ipdata.iphl = 255;

	struct nd_nbrsol nd_msg;
	nd_msg.res = 0;
	memcpy(nd_msg.tgtaddr, ip_llprefix, 16);
	nd_msg.tgtaddr[8] = 0x6e;
	nd_msg.tgtaddr[9] = 0xec;
	nd_msg.tgtaddr[10] = 0xeb;
	nd_msg.tgtaddr[11] = 0xff;
	nd_msg.tgtaddr[12] = 0xfe;
	nd_msg.tgtaddr[13] = 0xbb;
	nd_msg.tgtaddr[14] = 0x4a;
	nd_msg.tgtaddr[15] = 0x27;
	kprintf("TGT: ");
	print_ip(nd_msg.tgtaddr);

	int32 res = icmp_send(IF_ETH, ICMP_TYPE_NBRSOL, 0, &ipdata, (char *)&nd_msg, sizeof(struct nd_nbrsol));
	if(res == SYSERR)
		kprintf("SYSERR\n");
	else
		kprintf("HERE\n");
}

process print_ip(byte * ip)
{
	int32 i = 0;
	for(i = 0; i < 15; i++)
		kprintf("%02x:", ip[i]);

	kprintf("%02x\n", ip[15]);
}

/* process test_nd()
{
	byte ula[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	byte all_rtr[16] = {0};
	all_rtr[0] = 0xff;
	all_rtr[1] = 0x02;
	all_rtr[15] = 0x02;

	if_tab[IF_RADIO].if_ndData.sendadv = TRUE;
	/* memcpy(if_tab[IF_RADIO].if_ipucast[1].ipaddr, ula, 8);
	memcpy(&if_tab[IF_RADIO].if_ipucast[1].ipaddr[8], if_tab[IF_RADIO].if_hwucast, 8);
	if_tab[IF_RADIO].if_nipucast++;
	memcpy(if_tab[IF_RADIO].if_ipmcast[0].ipaddr, all_rtr, 16);
	if_tab[IF_RADIO].if_nipmcast = 1; 
	// char buf;
	// read(CONSOLE, &buf, 1);
	kprintf("sending nbr sol to 102\n");
	struct ipinfo ipdata;
	// memcpy(ipdata.ipsrc, if_tab[IF_RADIO].if_ipucast[0].ipaddr, 16);
	memcpy(ipdata.ipsrc, ip_llprefix, 8);
	memcpy(&ipdata.ipsrc[8], if_tab[IF_RADIO].if_hwucast, 8);
	memcpy(ipdata.ipdst, ip_solmc, 16);
	memset(&ipdata.ipdst[13], 0, 2);
	ipdata.ipdst[15] = 112;
	ipdata.iphl = 255;

	int32 i = 0;
	kprintf("DST: ");
	for(i = 0; i < 16; i++)
		kprintf("%02x:", ipdata.ipdst[i]);

	kprintf("\nSRC: ");

	for(i = 0; i < 16; i++)
		kprintf("%02x:", ipdata.ipsrc[i]);

	kprintf("\n");

	struct nd_nbrsol nbrsol;
	nbrsol.res = 0;
	kprintf("calling icmp_send now\n");
	int32 res = icmp_send(IF_RADIO, ICMP_TYPE_NBRSOL, 0, &ipdata, (char *)&nbrsol, sizeof(struct nd_nbrsol));
	if(res == SYSERR)
		kprintf("SYSERR\n");
	else
		kprintf("HERE\n");
} */
