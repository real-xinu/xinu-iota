/************************************************************************/
/*									*/
/*			Server-to-Node Protocol				*/
/*									*/
/* The following defines the protocol used between the testbed server	*/
/*	and nodes in the testbed.  Each message is sent in a TYPEA	*/
/*	Ethernet frame.  All integer values are sent in network byte	*/
/*	order.								*/
/*									*/
/************************************************************************/

/* Definition of message types */

#define	A_JOIN		0	/* A request to join the testbed that 	*/
				/*  that is broadcast by a node when it	*/
				/*  boots.  The message requests the	*/
				/*  server to respond with an A_ASSIGN	*/
				/*  that specifies an ID and a set of	*/
				/*  neighbors for the node.  If server	*/
				/*  chooses to omit the node from the	*/
				/*  test, the server responds with an	*/
				/*  A_ERR.  If it does not receive any	*/
				/*  response an A_JOIN within 40 ms, a	*/
				/*  node retransmits the A_JOIN, but	*/
				/*  only retransmits once.		*/

#define	A_ASSIGN	1	/* A response message sent to a node	*/
				/*  (i.e., unicast to the node) to	*/
				/*  assign the node an ID and specify	*/
				/*  a set of neighbors.  The neighbors	*/
				/*  are specified with a multicast	*/
				/*  address that the node should use	*/
				/*  when sending radio packets across	*/
				/*  the testbed Ethernet.  The node	*/
				/*  responds with an A_ACK (see below).	*/

#define	A_XOFF		2	/* A message sent by the server to	*/
				/*  terminate transmission.  After	*/
				/*  receipt, a node must discard all	*/
				/*  outgoing radio packets.  Typically,	*/
				/*  an A_XOFF will be broadcast to stop	*/
				/*  the emulation.  Note: a node that	*/
				/*  receives A_XOFF will still respond	*/
				/*  to A_PING and will still send an	*/
				/*  A_ACK if and A_ASSIGN arrives.	*/

#define	A_XON		3	/* A message sent by the server to	*/
				/*  resume outgoing transmission of	*/
				/*  radio packets.  A node may receive	*/
				/*  (and must honor) an A_ASSIGN message*/
				/*  between the time an A_XOFF arrives	*/
				/*  and the time an A_XON arrives.	*/

#define	A_RESTART	4	/* A message sent by the server to	*/
				/*  request a node to restart its	*/
				/*  protocol stack.  A sequence of	*/
				/*  A_XOFF, delay, A_RESTART can be	*/
				/*  used to emulate a node powering	*/
				/*  down and then restarting.  An	*/
				/*  A_RESTART can be sent to a single	*/
				/*  node or can be broadcast.		*/
				/*  Before responding to an A_RESTART,	*/
				/*  a node delays N msec, where N is	*/
				/*  the node ID it had previoulsy.	*/
				/*  The delay guarantees that the	*/
				/*  server will not be bombarded with	*/
				/*  simultaneous A_JOIN	messages.	*/



#define A_PING		5	/* A message sent by the server to test	*/
				/*  whether a node is alive (intended	*/
				/*  for debugging).  An A_PING is	*/
				/*  handled by netin, and does not	*/
				/*  require ICMPv6.  The node responds	*/
				/*  with an A_ACK.			*/

#define A_PING_ALL	6	/* A message broadcast by the server to	*/
				/*  request a response from all nodes.	*/
				/*  When an A_PING_ALL arrives, each	*/
				/*  node delays for N msec (where N is	*/
				/*  the node ID), and then responds	*/
				/*  with an A_ACK.			*/

#define A_PINGALL       7

#define	A_ACK		8	/* A response to an A_ASSIGN or A_PING	*/
				/*  to indicate that the message was	*/
				/*  processed.				*/

#define	A_ERR		9	/* A response sent by a node when one	*/
				/*  or more fields of an incoming	*/
				/*  message contains an invalid value.	*/

