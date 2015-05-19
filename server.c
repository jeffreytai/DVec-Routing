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

#define BUFSIZE 1024
#define PORTA 1
#define PORTB 2
#define PORTC 3
#define PORTD 4
#define PORTE 5
#define PORTF 6
#define PORTNOA 10000
#define PORTNOB 10001
#define PORTNOC 10002
#define PORTNOD 10003
#define PORTNOE 10004
#define PORTNOF 10005

struct entry {
	char 	dest;
	int 	cost;
	int 	outgoingPort;
	int 	destPort;
};

void error(char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	int 	sockfd[6]; /* socket */
	//int 	portno[6]; /* port to listen on */
	int 	clientlen; /* byte size of client's address */
	struct 	sockaddr_in serveraddr[6]; /* server's address */
	struct 	sockaddr_in clientaddr; /* client's address */
	struct 	hostent *hostp; /* client host info */
	struct 	hostent *serverp;
	char 	buf[BUFSIZE]; /* message buffer */
	char 	*hostaddrp; /* dotted decimal host address string */
	char 	*serveraddrp;
	int 	optval; /* flag value for setsockopt */
	int 	n; /* message byte size */

	struct entry table[6];

	/* check command line arguments */
	// if (argc < 2) {
	// 	fprintf(stderr, "Error, no port provided\n");
	// 	exit(1);
	// }

	/* create parent sockets */
	for (int i=0; i<6; i++) {
		if ( (sockfd[i] = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			error("Error opening socket");

		/* server can be rerun immediately after killed */
		optval = 1;
		setsockopt(sockfd[i], SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

		switch(i+1) {
			case PORTA:
				serveraddr[i].sin_port = 10000;
				break;
			case PORTB:
				serveraddr[i].sin_port = 10001;
				break;
			case PORTC:
				serveraddr[i].sin_port = 10002;
				break;
			case PORTD:
				serveraddr[i].sin_port = 10003;
				break;
			case PORTE:
				serveraddr[i].sin_port = 10004;
				break;
			case PORTF:
				serveraddr[i].sin_port = 10005;
				break;
			default:
				break;

			bzero((char *) &serveraddr[i], sizeof(serveraddr[i]));
			serveraddr[i].sin_family = AF_INET;
			serveraddr[i].sin_addr.s_addr = htonl(INADDR_ANY);
			
			if ( bind(sockfd[i], (struct sockaddr *) &serveraddr[i], sizeof(serveraddr[i])) < 0 )
				error("Error on binding");
		}
	}

	for (int i=0; i<6; i++) {
		printf("Port no: %hu\n", serveraddr[i].sin_port);
	}

	
	// sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	// if (sockfd < 0)
	// 	error("Error opening socket");

	/* server can be rerun immediately after killed */
	// optval = 1;
	// setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

	/* build server's Internet address */
	// bzero((char *) &serveraddr, sizeof(serveraddr));
	// serveraddr.sin_family = AF_INET;
	// serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	// serveraddr.sin_port = htons((unsigned short)portno);

	// /* bind: associate parent socket with port */
	// if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	// 	error("Error on binding");
	
	/* forwarding table entries */
	struct entry entryA = { 'A', 0, 10000, 10000 };
	struct entry entryB = { 'B', 3, 10000, 10001 };
	struct entry entryC = { 'C', NULL, NULL, NULL };
	struct entry entryD = { 'D', NULL, NULL, NULL };
	struct entry entryE = { 'E', 1, 10000, 10005 };
	struct entry entryF = { 'F', NULL, NULL, NULL};
	table[0] = entryA;
	table[1] = entryB;
	table[2] = entryC;
	table[3] = entryD;
	table[4] = entryE;
	table[5] = entryF;

	for (int i=0; i<6; i++) {
		printf("Minimum distance to node %c at port %i costs %i through port %i\n", table[i].dest, table[i].destPort, table[i].cost, table[i].outgoingPort);
	}

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
		printf("Node received datagram from Port %d. Datagram sent to Port %d\n", ntohs(clientaddr.sin_port), ntohs(serveraddr[0].sin_port));

		/* echo input back to client */
		n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
		if (n < 0)
			error("Error sending to client");
	}
}