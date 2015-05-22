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

#define BUFSIZE 	128
#define ROUTERA 	10000
#define ROUTERB		10001
#define ROUTERC		10002
#define ROUTERD		10003
#define ROUTERE		10004
#define ROUTERF		10005
#define INDEXA		0
#define INDEXB		1
#define INDEXC		2
#define INDEXD		3
#define INDEXE		4
#define INDEXF		5
#define NUMROUTERS	6


#ifndef max
	#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

// struct node is size 80
struct router
{
	int index;
	char otherRouters[6];
	int costs[6];
	int outgoingPorts[6];
	int destinationPorts[6];
};

void error(char *msg) {
	perror(msg);
	exit(1);
}

void updateTable(struct router *currTable, struct router rcvdTable)
{
	for (int i=0; i<NUMROUTERS; i++) {
		// ignore own entry in table
		if (i != currTable->index) {
			// get information on previously unknown routers
			if (currTable->costs[i] == NULL && rcvdTable.costs[i] != NULL && rcvdTable.costs[i] != 0) {
				currTable->costs[i] = rcvdTable.costs[i] + currTable->costs[rcvdTable.index];
				currTable->destinationPorts[i] = rcvdTable.destinationPorts[i];
				currTable->outgoingPorts[i] = rcvdTable.index + 10000;
			}
			// // find shorter paths to other routers
			// for (int j=0; j<NUMROUTERS, j++) {
			// 	if (i != j) {
			// 		if (currTable.costs[i] > currTable.costs[rcvdTable.index] + rcvdTable.costs[i])
			// 	}
			// }
		}
	}
	return;
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

	struct router tableA = {
		INDEXA,
		{   'A', 	 'B', 	'C',   'D',   'E',    'F' },
		{    0, 	  3, 	NULL, NULL,    1, 	 NULL },
		{ ROUTERA, ROUTERA, NULL, NULL, ROUTERA, NULL },
		{ ROUTERA, ROUTERB, NULL, NULL, ROUTERE, NULL }
	};

	struct router tableB = {
		INDEXB,
		{   'A', 	 'B', 	  'C', 	 'D',	 'E', 	   'F'  },
		{ 	 3, 	  0, 	   3, 	 NULL, 	  2, 	   1    },
		{ ROUTERB, ROUTERB, ROUTERB, NULL, ROUTERB, ROUTERB },
		{ ROUTERA, ROUTERB, ROUTERC, NULL, ROUTERE, NULL    }
	};

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
			case ROUTERA:
				serveraddr[i].sin_port = htons(ROUTERA);
				break;
			case ROUTERB:
				serveraddr[i].sin_port = htons(ROUTERB);
				break;
		}
		/* bind: associate parent socket with port */
		if (bind(sockfd[i], (struct sockaddr *) &serveraddr[i], sizeof(serveraddr[i])) < 0)
			error("Error on binding");
	}

	clientlen = sizeof(clientaddr);

	int nsocks = max(sockfd[0], sockfd[1]) + 1;

	int test=1;
	/* loop: wait for datagram, then echo it */
	while (test) {
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
				// memset(&buf, 0, BUFSIZE);
				// memcpy(buf, &tableA, sizeof(struct router));
				
				/* receives UDP datagram from client */
				hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
				if (hostp == NULL)
					error("Error getting host by address");
				/* Converts address from network bytes to IPv4 decimal notation string */
				hostaddrp = inet_ntoa(clientaddr.sin_addr);
				if (hostaddrp == NULL)
					error ("Error on ntoa");
				printf("Router received datagram from Port %d. Datagram sent to Port %d\n", ntohs(clientaddr.sin_port), ntohs(serveraddr[0].sin_port));
				// printf("Buffer contains %d bytes: %s\n", sizeof(router), buf);

				struct router *compTable = NULL;
				compTable = malloc(BUFSIZE);
				memcpy(compTable, buf, sizeof(struct router));
				for (int x=0; x<6; x++)
					printf("compTable elements - dest: %c, cost: %i, outgoingPorts: %i, destinationPorts: %i\n", compTable->otherRouters[x], compTable->costs[x], compTable->outgoingPorts[x], compTable->destinationPorts[x]);

				printf("Original Table A\n");
				for (int x=0; x<6; x++)
					printf("table A elements - dest: %c, cost: %i, outgoingPorts: %i, destinationPorts: %i\n", tableA.otherRouters[x], tableA.costs[x], tableA.outgoingPorts[x], tableA.destinationPorts[x]);

				updateTable(&tableA, *compTable);
				printf("Updated Table A\n");
				for (int x=0; x<6; x++)
					printf("table A elements - dest: %c, cost: %i, outgoingPorts: %i, destinationPorts: %i\n", tableA.otherRouters[x], tableA.costs[x], tableA.outgoingPorts[x], tableA.destinationPorts[x]);
				/* echo input back to client */
				//n = sendto(sockfd[0], buf, sizeof(router), 0, (struct sockaddr *)&clientaddr, clientlen);
				n = sendto(sockfd[0], buf, sizeof(struct router), 0, (struct sockaddr *)&serveraddr[1], clientlen);
				if (n < 0)
					error("Error sending to client");

				break;
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

				memset(&buf, 0, BUFSIZE);
				memcpy(buf, &tableB, sizeof(struct router));


				/* echo input back to client */
				n = sendto(sockfd[1], buf, sizeof(struct router), 0, (struct sockaddr *)&serveraddr[0], clientlen);
				if (n < 0)
					error("Error sending to client");
			}
		}
	}
}