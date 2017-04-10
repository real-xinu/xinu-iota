/* wireshark_server */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <fcntl.h>

#define	WIRESHARK_PORT	50000
#define	WIRESHARK_FIFO	"wfifo"

#define	PCAP_MAGIC	0xa1b2c3d4;
#define	PCAP_NETWORK	230

#pragma pack(1)
struct	pcap_hdr {
	uint32_t	magic;
	uint16_t	vers_maj;
	uint16_t	vers_min;
	int32_t		thiszone;
	uint32_t	sigfigs;
	uint32_t	snaplen;
	uint32_t	network;
};

struct	pcap_pkthdr {
	uint32_t	ts_sec;
	uint32_t	ts_usec;
	uint32_t	incl_len;
	uint32_t	orig_len;
};

struct	monitor_msg {
	uint32_t	pktlen;
	uint8_t		pktdata[];
};
#pragma pack()

int	main (void) {

	struct	monitor_msg *mmsg;
	struct	addrinfo hints, *result;
	struct	pcap_hdr pcaphdr;
	struct	pcap_pkthdr pcapphdr;
	char	*buf, portstr[10];
	int	sockfd;
	int	wfd;
	int	err;

	buf = (char *)malloc(1500);
	if(!buf) {
		printf("Cannot allocate memory\n");
		return 1;
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1) {
		perror("socket(): ");
		return 1;
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	sprintf(portstr, "%d", WIRESHARK_PORT);

	err = getaddrinfo(NULL, portstr, &hints, &result);
	if(err != 0) {
		printf("getaddrinfo(): %s\n", gai_strerror(err));
		close(sockfd);
		return 1;
	}

	err = bind(sockfd, result->ai_addr, result->ai_addrlen);
	if(err == -1) {
		perror("bind(): ");
		close(sockfd);
		freeaddrinfo(result);
		return 1;
	}

	freeaddrinfo(result);

	wfd = open(WIRESHARK_FIFO, O_RDWR);
	if(wfd == -1) {
		perror("open(): ");
		close(sockfd);
		return 1;
	}

	pcaphdr.magic = PCAP_MAGIC;
	pcaphdr.vers_maj = 2;
	pcaphdr.vers_min = 4;
	pcaphdr.thiszone = 0;
	pcaphdr.sigfigs = 0;
	pcaphdr.snaplen = 65535;
	pcaphdr.network = PCAP_NETWORK;

	pcapphdr.ts_sec = 0;
	pcapphdr.ts_usec = 0;

	while(1) {

		err = recvfrom(sockfd, buf, 1500, 0, NULL, NULL);
		if(err == -1) {
			perror("recvfrom(): ");
			close(sockfd);
			return 1;
		}

		mmsg = (struct monitor_msg *)buf;
		mmsg->pktlen = ntohl(mmsg->pktlen);

		printf("Received a message with packet length: %d\n", mmsg->pktlen);
		pcapphdr.incl_len = mmsg->pktlen;
		pcapphdr.orig_len = mmsg->pktlen;

		write(wfd, &pcapphdr, sizeof pcapphdr);
		write(wfd, mmsg->pktdata, mmsg->pktlen);
	}

	close(sockfd);
	return 0;
}
