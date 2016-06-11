#include <xinu.h>
#define PORT 55000
#define TRUE 1

struct c_msg *   cmsg_handler(int32 mgm_msgtyp)
{
         struct c_msg *cmsg_reply;
	 cmsg_reply = (struct c_msg *)getmem(sizeof(struct c_msg));
         memset(cmsg_reply, 0, sizeof(struct c_msg));
         
         switch(mgm_msgtyp)
	 {
            case C_RESTART:
		    kprintf("Message type is %d\n", C_RESTART);
		    cmsg_reply->cmsgtyp = htonl(C_OK); 
		    break;
            case C_RESTART_NODES:
		    kprintf("Message type is %d\n", C_RESTART_NODES);
		    cmsg_reply->cmsgtyp = htonl(C_OK); 
		    break;
	    case C_PING_REQ:
		    kprintf("Message type is %d\n", C_PING_REQ);
		    cmsg_reply->cmsgtyp = htonl(C_OK);
		    break;
	    case C_PING_ALL:
		    kprintf("Message type is %d\n", C_PING_ALL);
		    cmsg_reply->cmsgtyp = htonl(C_OK);
		    break;
	    case C_XOFF:
		    kprintf("Message type is %d\n", C_XOFF);
		    cmsg_reply->cmsgtyp = htonl(C_OK); 
		    break;
	    case C_XON:
		    kprintf("Message type is %d\n", C_XON);
		    cmsg_reply->cmsgtyp = htonl(C_OK); 
		    break;
	    case C_OFFLINE:
		    kprintf("Message type is %d\n", C_OFFLINE);
		    cmsg_reply->cmsgtyp = htonl(C_OK); 
		    break;
	    case C_ONLINE:
		    kprintf("Message type is %d\n", C_ONLINE);
		    cmsg_reply->cmsgtyp = htonl(C_OK); 
		    break;
	    case C_TOP_REQ:
		    kprintf("Message type is %d\n", C_TOP_REQ);
		    cmsg_reply->cmsgtyp = htonl(C_OK); 
		    break;
            case C_NEW_TOP:
		    kprintf("Message type is %d\n", C_NEW_TOP);
		    cmsg_reply->cmsgtyp = htonl(C_OK); 
		    break;
	    case C_TS_REQ:
		    kprintf("Message type is %d\n", C_TS_REQ);
		    cmsg_reply->cmsgtyp = htonl(C_TS_RESP);
		    break;


	 }
         return cmsg_reply;


}


process stomgmt()
{

  uint32 remip = 0; //2148173875u;
  uint16 remport = PORT;
  uint16 locport = PORT;
  char *recvbuff = getmem(sizeof(struct c_msg)); /* Allocate memory to the receving buffer */
  char *sendbuff;// = getmem(sizeof(struct c_msg)); 


  uid32 udp_id;
  udp_id = udp_register(remip, remport, locport); /* create a UDP process on port 55000 */



  while(TRUE)
  {
     int32 retval;
     retval = udp_recvaddr(udp_id, &remip, &remport, recvbuff, sizeof(struct c_msg), 100);

     struct c_msg *mgmt_msg;
     struct c_msg *reply;
     mgmt_msg = (struct c_msg *)recvbuff;
     int32 mgm_msgtyp =  ntohl(mgmt_msg->cmsgtyp);
          
     if ((retval > 0))
     {
       reply = cmsg_handler(mgm_msgtyp);  //Call c_msg handler 
       kprintf("reply: %d\n", ntohl(reply->cmsgtyp));
       sendbuff = (char *)reply;
       status stat = udp_sendto(udp_id, remip, remport, sendbuff, sizeof(struct c_msg));
       if(stat > 0)
       {
           kprintf("The reply from testbed server is sent to mgmt server\n");

       }
     }    
  }

/*

  while(TRUE)
  {
     int32 retval;
     retval = udp_recv(udp_id, recvbuff, sizeof(struct c_msg), 100);
     
     struct c_msg *mgmt_msg;
     struct c_msg *reply;
     mgmt_msg = (struct c_msg *)recvbuff;
     int32 mgm_msgtyp =  ntohl(mgmt_msg->cmsgtyp);
          
     if ((retval > 0))
     {
       reply = cmsg_handler(mgm_msgtyp);  //Call c_msg handler 
       kprintf("reply: %d\n", ntohl(reply->cmsgtyp));
       sendbuff = (char *)reply; 
       status stat = udp_send(udp_id, sendbuff, sizeof(struct c_msg));
       if(stat > 0)
       {
           kprintf("The reply from testbed server is sent to mgmt server\n");

       }
     }

  }
*/
}



int32  stonode_init()
{

    resume(create(stomgmt, 8192, 30, "stomgmt", 0, NULL));
    /*struct etherPkt *msg;
    msg = (struct etherPkt *)getbuf(netbufpool);
    int32  msglen, retval;
    msglen  = ETH_HDR_LEN;*/

    /* fill out Ethernet packet fields */
    /*memset(msg, 0, sizeof(msg));
    memcpy(msg->src, NetData.ethucast, ETH_ADDR_LEN);
    memcpy(msg->dst, NetData.ethbcast, ETH_ADDR_LEN);
    msg->type = htons(ETH_TYPE_A);
    msg->amsgtyp = htonl(A_RESTART);*/


    /* send packet over Ethernet */
    /*retval = write(ETHER0,(char *)msg, msglen);
    if(retval == SYSERR)
    {
         return SYSERR;

    }else
    {        
         return OK;

    }*/
    return OK;

}

int32 node_assign(struct netpacket *pkt)
{
    struct etherPkt *assign_msg;
    assign_msg = (struct etherPkt *)getbuf(netbufpool);
    int32 msglen, retval;
    msglen = ETH_HDR_LEN;

    /*fill out the Ethernet packet fields */
    memset(assign_msg, 0, sizeof(assign_msg));
    memcpy(assign_msg->src, NetData.ethucast, ETH_ADDR_LEN);
    memcpy(assign_msg->dst, pkt->net_ethsrc, ETH_ADDR_LEN);
    assign_msg->type = htons(ETH_TYPE_A);
    assign_msg->amsgtyp = htons(A_ASSIGN);
    assign_msg->anodeid = htonl(1);

    retval = write(ETHER0, (char *)assign_msg, msglen);
    return retval;


}
void stonode(struct netpacket *pkt)
{
   struct etherPkt *server_msg;
   int32 retval;
   server_msg = (struct etherPkt *)pkt->net_udpdata;
   int32 amsgtyp = ntohs(server_msg->amsgtyp);

   switch(amsgtyp)
   {
     case A_JOIN:
	     kprintf("Join message is received\n");
	     retval = node_assign(pkt);
	     if (retval == OK)
	     {
                kprintf("Assign message is sent\n");

	     }
	     break;
     case A_ACK:
	     kprintf("ACK is received\n");
	     break;
     case A_ERR:
             break;


   }
  

}

