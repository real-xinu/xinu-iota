/* wsserver.c - Wi-SUN emulation testbed server */

#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>



int xon = 1;
int32 slot;
struct node_info info;

/*--------------------------------------------------------
 * Process TYPE_B frames to check multicast address
 * ------------------------------------------------------*/
void process_typb (struct netpacket_e *pkt)
{
    int i;
    if (xon) {

	
      if (srbit (pkt->net_ethdst, info.nodeid, 1 )) {
            uint32 t1 = clktime;
	    printf("t1:%d\n", t1);
            for (i = 0; i < 6; i++) {
                kprintf ("%02x:", pkt->net_ethsrc[i]);
            }

            kprintf ("\n");

            for (i = 0; i < 6; i++) {
                //kprintf ("%02x:", pkt->net_ethdst[i]);
            }

            //kprintf ("\ndata:%s\n", pkt->net_ethdata);
      } 
        freebuf ((char *)pkt);

    } else if (!xon) {
        freebuf ((char *)pkt);
    }
    kprintf("\n*************************\n");

}

/*-------------------------------------------------------------
 *  Send radio packets between the nodes using TYPE_B frames
 *-----------------------------------------------------------*/

process wsnodeapp()
{
    int32 retval;
    int j;
    /* DEBUG */
    /*
        for (j = 0; j < 6; j++) {
            //kprintf("%02x", info.mcastaddr[j]);
        }

        //kprintf("\n");
        for (j = 0; j < 6; j++) {
            //    kprintf("%02x", NetData.ethucast[j]);
        }

    kprintf("\n");
    */
    while (TRUE) {
        sleepms (200);
        int flag = 0;
        //kprintf ("xon:%d\n", xon);

        if (xon) {
            if (info.mcastaddr[0] == 1) {
                flag++;

                for (j = 1; j < 6; j++) {
                    if (info.mcastaddr[j] == 0)
                        flag++;
                }

            } else if (info.mcastaddr[0] == 0)
                flag = 6;

            if (flag != 6) {
                struct netpacket_e *pkt;
		struct netpacket   *npkt;
		npkt = (struct netpacket *)getmem(sizeof(struct netpacket));
                char *buf;
		//memset(npkt, 0 , PACKLEN);
		//npkt->net_iface = 1;
		//npkt->net_ipvtch = 0x60;
		//npkt->net_ipnh = IP_ICMP;
		//npkt->net_iphl = 255;
		//npkt->net_iplen = 4 + 50;
		
                buf = (char *)getmem (50 * sizeof (char));
                //buf = info.nodeid;
                sprintf (buf, "%d", info.nodeid);
                memcpy (npkt->net_icdata, buf, 50 * sizeof (char) );
                pkt = ( struct netpacket_e *) getmem ( sizeof ( struct netpacket_e) );
                memcpy (pkt->net_ethsrc, NetData.ethucast, ETH_ADDR_LEN );
                memcpy (pkt->net_ethdst, info.mcastaddr, ETH_ADDR_LEN );
                pkt->net_ethtype = htons (ETH_TYPE_B);
                memcpy(pkt->net_ethdata, npkt, 1500 - 44);
                retval = write ( ETHER0, ( char * )pkt, sizeof ( struct netpacket_e) );

                if (retval > 0) {
                }

                freemem ((char *) pkt , sizeof (struct netpacket_e));
                freemem ((char *)buf, sizeof (char) * 50);
            }
        }
    }


    
}


/*-----------------------------------------------------------
* Create and Ethernet packet and fill out header fields
*----------------------------------------------------------*/
struct netpacket_e *create_etherPkt ( struct netpacket_e *pkt )
{
    struct netpacket_e *msg;
    msg = ( struct netpacket_e * ) getmem ( sizeof ( struct netpacket_e) );
    /*fill out Ethernet packet header fields */
    memset ( msg, 0, sizeof ( msg ) );
    memcpy ( msg->net_ethsrc, NetData.ethucast, ETH_ADDR_LEN );
    memcpy ( msg->net_ethdst, pkt->net_ethsrc, ETH_ADDR_LEN );
    msg->net_ethtype = htons ( ETH_TYPE_A );
    return msg;
}


