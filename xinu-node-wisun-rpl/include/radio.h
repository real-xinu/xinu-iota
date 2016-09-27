/* radio.h */

#define	NBR_STATE_FREE	0
#define	NBR_STATE_USED	1

#define	NBRTAB_SIZE	10

struct	nbrentry {
	byte	state;
	byte	hwaddr[8];
	uint32	txattempts;
	uint32	ackrcvd;
	uint16	etx;
};

/* State of radio interface */
#define RAD_STATE_DOWN	0
#define	RAD_STATE_UP	1

/* Length of radio address */
#define	RAD_ADDR_LEN	8

#define	RAD_TX_RING_SIZE	32

#define	RAD_CTRL_GET_HWADDR	1

struct	radcblk	{
	byte	state; 		/* RAD_STATE_... as defined above 	*/
	struct	dentry	*phy;	/* physical eth device for Tx DMA 	*/
	byte 	type; 		/* NIC type_... as defined above 	*/

	/* Pointers to associated structures */

	struct	dentry	*dev;	/* address in device switch table	*/
	void	*csr;		/* addr.of control and status regs.	*/
	uint32	pcidev;		/* PCI device number			*/
	uint32	iobase;		/* I/O base from config			*/
	uint32  flashbase;      /* flash base from config	       	*/
    	uint32	membase; 	/* memory base for device from config	*/

	void    *rxRing;	/* ptr to array of recv ring descriptors*/
	void    *rxBufs; 	/* ptr to Rx packet buffers in memory	*/
	uint32	rxHead;		/* Index of current head of Rx ring	*/
	uint32	rxTail;		/* Index of current tail of Rx ring	*/
	uint32	rxRingSize;	/* size of Rx ring descriptor array	*/
	uint32	rxIrq;		/* Count of Rx interrupt requests       */

	void    *txRing; 	/* ptr to array of xmit ring descriptors*/
	void    *txBufs; 	/* ptr to Tx packet buffers in memory	*/
	uint32	txHead;		/* Index of current head of Tx ring	*/
	uint32	txTail;		/* Index of current tail of Tx ring	*/
	uint32	txRingSize;	/* size of Tx ring descriptor array	*/
	uint32	txIrq;		/* Count of Tx interrupt requests       */

	byte	devAddress[RAD_ADDR_LEN];/* EUI-64 address 		*/

	uint8	addrLen;	/* Hardware address length	      	*/
	uint16	mtu;	    	/* Maximum transmission unit (payload)  */

	uint32	errors;		/* Number of Ethernet errors 		*/
	sid32	isem;		/* Semaphore for Ethernet input		*/
	sid32	osem; 		/* Semaphore for Ethernet output	*/
	sid32	qsem;
	uint16	istart;		/* Index of next packet in the ring     */

	int16	inPool;		/* Buffer pool ID for input buffers 	*/
	int16	outPool;	/* Buffer pool ID for output buffers	*/

	int16 	proms; 		/* nonzero => promiscuous mode 		*/

	pid32	ro_process;	/* Radio output process			*/

	struct	nbrentry nbrtab[NBRTAB_SIZE];
};

extern	struct	radcblk	radtab[];	/* array of control blocks      */

struct	rad_tx_ring_desc {
	void	*data;		/* Packet data	*/
	int32	datalen;	/* Data length	*/
};
