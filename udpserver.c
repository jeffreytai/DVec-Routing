#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <algorithm>  /* necessary for max() */

#include <sys/time.h>
#include <sys/select.h>

#define BUFSIZE 1024
#define PORTA 10000
#define PORTB 10001
// #define PORTC 10002
// #define PORTD 10003
// #define PORTE 10004
// #define PORTF 10005

struct table {
	char 	dest[6];
	int 	cost[6];
	int 	outgoingPort[6];
	int 	destPort[6];
};

void error(char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	int 	sockfd[2]; /* socket */
	int 	portno; /* port to listen on */
	int 	clientlen; /* byte size of client's address */
	struct 	sockaddr_in serveraddr[2]; /* server's address */
	struct 	sockaddr_in clientaddr; /* client's address */
	struct 	hostent *hostp; /* client host info */
	char 	buf[BUFSIZE]; /* message buffer */
	char 	*hostaddrp; /* dotted decimal host address string */
	int 	optval; /* flag value for setsockopt */
	int 	n; /* message byte size */

	//struct table forwardingTable[6];

	/* check command line arguments */
	if (argc < 2) {
		fprintf(stderr, "Error, no port provided\n");
		exit(1);
	}
	portno = atoi(argv[1]);

	for (int i=0; i<2; i++) {
		if ( (sockfd[i] = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
			error("Error opening socket");

		optval = 1;
		setsockopt(sockfd[i], SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

		switch(i+10000) {
			case PORTA:
				serveraddr[i].sin_port = htons(10000);
				break;
			case PORTB:
				serveraddr[i].sin_port = htons(10000);
				break;
			default:
				break;
		}

		bzero((char *)&serveraddr[i], sizeof(serveraddr[i]));
		serveraddr[i].sin_family = AF_INET;
		serveraddr[i].sin_addr.s_addr = htonl(INADDR_ANY);

		if ( bind(sockfd[i], (struct sockaddr *)&serveraddr[i], sizeof(serveraddr[i])) < 0)
			error("Error on binding");
	}

	fd_set socks;
	FD_ZERO(&socks);
	FD_SET(sockfd, &socks);
	FD_SET(sockfd2, &socks);

	int nsocks = max(sockfd, sockfd2) + 1;
	
	if (select(nsocks, &socks, (fd_set *)0, (fd_set *)0, 0) >= 0) {
		if (FD_ISSET(sockfd, &socks)) {
			n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
			if (n < 0)
				error("Error receiving datagram from client");
		}
		if (FD_ISSET(sockfd2, &socks)) {
			n = recvfrom(sockfd2, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
			if (n < 0)
				error("Error receiving datagram from client");
		}
	}

	/* create parent sockets */
	// sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	// if (sockfd < 0)
	// 	error("Error opening socket");

	// /* server can be rerun immediately after killed */
	// optval = 1;
	// setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

	// /* build server's Internet address */
	// bzero((char *) &serveraddr, sizeof(serveraddr));
	// serveraddr.sin_family = AF_INET;
	// serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	// serveraddr.sin_port = htons((unsigned short)portno);

	// /* bind: associate parent socket with port */
	// if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	// 	error("Error on binding");

	
	

	/* loop: wait for datagram, then echo it */
	clientlen = sizeof(clientaddr);
	while (1) {
		/* receives UDP datagram from client */
		bzero(buf, BUFSIZE);

		n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
		if (n < 0)
			error("Error receiving datagram from client");
		/* determine who sent the datagram */
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
			error("Error getting host by address");
		/* Converts address from network bytes to IPv4 decimal notation string */
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			error ("Error on ntoa");
		printf("Node received datagram from Port %d. Datagram sent to Port %d\n", ntohs(clientaddr.sin_port), ntohs(serveraddr.sin_port));

		/* echo input back to client */
		n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
		if (n < 0)
			error("Error sending to client");
	}
}