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

	struct netpacket *pkt;

	sleep(3);

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

			read(CONSOLE, &c, 1);
			read(CONSOLE, &c, 1);

			pkt = (struct netpacket *)getbuf(netbufpool);

			byte	ipdst[] = {0x20, 0x01, 0x00, 0x00, 0x0, 0x00, 0x00, 0x00, 0x6E, 0xEC, 0xEB, 0xFF, 0xFE, 0xAD, 0x68, 0xF2};

			memcpy(ip_fwtab[1].ipfw_prefix.ipaddr, ipdst, 16);
			ip_fwtab[1].ipfw_prefix.prefixlen = 8;
			ip_fwtab[1].ipfw_iface = 1;
			ip_fwtab[1].ipfw_onlink = 0;
			ip_fwtab[1].ipfw_state = 1;

			memset(pkt, 0, 40);

			pkt->net_ipvtch = 0x60;
			pkt->net_iplen = 100;
			pkt->net_ipnh = 0xed;
			pkt->net_iphl = 0xff;
			memcpy(pkt->net_ipsrc, iftab[1].if_ipucast[1].ipaddr, 16);
			memcpy(pkt->net_ipdst, ipdst, 16);
			//kprintf("Sending to C...");

			//ip_send(pkt);

			char	data[] = "Test Data";
			char	data2[10];
			int32	icmpslot;
			int32	retval;

			icmpslot = icmp_register(ICMP_TYPE_ERP, 0, ipdst, ip_unspec, 1);
			kprintf("ICMP slot: %d\n", icmpslot);

			icmp_send(ICMP_TYPE_ERQ, 0, iftab[1].if_ipucast[1].ipaddr, ipdst, data, strlen(data), 1);
			retval = icmp_recv(icmpslot, data2, 10, 4000);
			if(retval == SYSERR) {
				kprintf("ICMP recv error\n");
			}
			else if(retval == TIMEOUT) {
				kprintf("ICMP timeout\n");
			}
			else {
				data[10] = '\0';
				kprintf("Received data: %s\n", data2);
			}
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
