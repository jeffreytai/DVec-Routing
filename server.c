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

#include <sys/time.h>   /* For FD_SET, FD_SELECT */
#include <sys/select.h>

#define BUFSIZE 1024
#define PORTA 	10000
#define PORTB	10001
#define PORTC	10002
#define PORTD	10003
#define PORTE	10004
#define PORTF	10005

#ifndef max
	#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

struct node
{
	char nodes[6];
	int costs[6];
	int outgoingPorts[6];
	int destinationPorts[6];
};

void error(char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	int sockfd[2]; /* socket */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr[2]; /* server's address */
	struct sockaddr_in clientaddr; /* client's address */
	struct hostent *hostp; /* client host info */
	char buf[BUFSIZE]; /* message buffer */
	char *hostaddrp; /* dotted decimal host address string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */

	fd_set socks;

	for (int i=0; i<2; i++) {
		/* create parent socket */
		if ( (sockfd[i] = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
			error("Error opening socket");

		/* server can be rerun immediately after killed */
		optval = 1;
		setsockopt(sockfd[i], SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

		/* build server's Internet address */
		bzero((char *) &serveraddr[i], sizeof(serveraddr[i]));
		serveraddr[i].sin_family = AF_INET;
		serveraddr[i].sin_addr.s_addr = htonl(INADDR_ANY);

		switch(i+10000) {
			case PORTA:
				serveraddr[i].sin_port = htons(PORTA);
				break;
			case PORTB:
				serveraddr[i].sin_port = htons(PORTB);
				break;
		}
		/* bind: associate parent socket with port */
		if (bind(sockfd[i], (struct sockaddr *) &serveraddr[i], sizeof(serveraddr[i])) < 0)
			error("Error on binding");
	}

	clientlen = sizeof(clientaddr);

	int nsocks = max(sockfd[0], sockfd[1]) + 1;

	/* loop: wait for datagram, then echo it */
	while (1) {
		FD_ZERO(&socks);
		FD_SET(sockfd[0], &socks);
		FD_SET(sockfd[1], &socks);

		if (select(nsocks, &socks, NULL, NULL, NULL) < 0) {
			printf("Error selecting socket\n");
		} else {
			/* receives UDP datagram from client */
			bzero(buf, BUFSIZE);
			if (FD_ISSET(sockfd[0], &socks)) {
				if ( (n = recvfrom(sockfd[0], buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");
				
				/* receives UDP datagram from client */
				hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
				if (hostp == NULL)
					error("Error getting host by address");
				/* Converts address from network bytes to IPv4 decimal notation string */
				hostaddrp = inet_ntoa(clientaddr.sin_addr);
				if (hostaddrp == NULL)
					error ("Error on ntoa");
				printf("Router received datagram from Port %d. Datagram sent to Port %d\n", ntohs(clientaddr.sin_port), ntohs(serveraddr[0].sin_port));
				printf("Router received %d/%d bytes: %s\n", strlen(buf), n, buf);

				/* echo input back to client */
				n = sendto(sockfd[0], buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
				if (n < 0)
					error("Error sending to client");
			} else if (FD_ISSET(sockfd[1], &socks)) {
				if ( (n = recvfrom(sockfd[1], buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");
				hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
				if (hostp == NULL)
					error("Error getting host by address");
				/* Converts address from network bytes to IPv4 decimal notation string */
				hostaddrp = inet_ntoa(clientaddr.sin_addr);
				if (hostaddrp == NULL)
					error ("Error on ntoa");
				printf("Router received datagram from Port %d. Datagram sent to Port %d\n", ntohs(clientaddr.sin_port), ntohs(serveraddr[1].sin_port));
				printf("Router received %d/%d bytes: %s\n", strlen(buf), n, buf);

				/* echo input back to client */
				n = sendto(sockfd[1], buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
				if (n < 0)
					error("Error sending to client");
			}
		}
	}
}