/* radio.h */

#define RAD_HDR_LEN	23	/* Maximum 802.15.4 header length	*/
#define RAD_ADDR_LEN	8	/* EUI64 address length			*/
#define RAD_MTU		128	/* 802.15.4 MTU				*/

#define RAD_FCS_TYPE	0
#define RAD_FCS_SEQ	0
#define RAD_FCS_DAM	2
#define RAD_FCS_VER	4
#define RAD_FCS_SAM	6

#define RAD_TYPE_DATA	1

#define RAD_PKT_SIZE	(RAD_HDR_LEN + 5 + RAD_MTU)

struct	radpacket {
	byte	rad_data[RAD_PKT_SIZE];
};

/* Fragment table states */

#define RAD_FRAG_FREE	0
#define RAD_FRAG_USED	1

/* Number of partially assembled packets */

#define RAD_NFRAG	5

/* State of Radio device */

#define RAD_STATE_FREE	0
#define RAD_STATE_DOWN	1
#define RAD_STATE_UP	2

/* Radio device control functions */

#define RAD_CTRL_GET_EUI64	1

/* Control block for Radio device */

struct	radcblk {
	byte	state;		/* RAD_STATE_... as defined above	*/

	/* Pointers to associated structures */

	struct	dentry *dev;	/* Address in device switch table	*/
	void	*csr;		/* Control and status register address	*/

	void	*rxRing;	/* Ptr to array of recv ring descriptors*/
	void	*rxBufs;	/* Ptr to Rx packet buffers in memory	*/
	uint32	rxHead;		/* Index of current head of Rx ring	*/
	uint32	rxTail;		/* Index of current tail of Rx ring	*/
	uint32	rxRingSize;	/* Size of Rx ring descriptor array	*/
	uint32	rxIrq;		/* Count of Rx interrupt requests	*/

	void	*txRing;	/* Ptr to array of xmit ring descriptors*/
	void	*txBufs;	/* Ptr to Tx packet buffers in memory	*/
	uint32	txHead;		/* Index of current head of Tx ring	*/
	uint32	txTail;		/* Index of current tail of Tx ring	*/
	uint32	txRingSize;	/* Size of Tx ring descriptor array	*/
	uint32	txIrq;		/* Count of Tx interrupt requests	*/

	byte	devAddress[RAD_ADDR_LEN];/* EUI64 address		*/
	uint16	mtu;		/* Maximum Transmission Unit (payload)	*/

	sid32	isem;		/* Semaphore for input			*/
	sid32	osem;		/* Semaphore for output			*/

	int16	inPool;		/* Buffer pool ID for input buffers	*/
	int16	outPool;	/* Buffer pool ID for output buffers	*/

	bool8	_6lowpan;	/* Flag to indicate of 6LoWPAN use	*/
};

extern	struct radcblk radtab[];	/* Array of control blocks	*/

struct	rad_fragentry {
	byte	state;		/* Slot is free or used		*/
	byte	dsteui64[8];	/* Destination EUI64 address	*/
	byte	srceui64[8];	/* Source EUI64 address		*/
	uint16	dsize;		/* Datagram size		*/
	uint16	dtag;		/* Fragmentation tag		*/
	byte	dnxtoff;	/* Next expected offset		*/
	//struct	netpacket pkt;	/* Buffer to hold final packet	*/
};

extern	struct rad_fragentry rad_fragtab[];
