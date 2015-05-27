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
#include <stdbool.h> /* boolean */
#include <limits.h> /* INT_MAX */

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

bool stableState() {
	//
	return false;
}

/* currently unused */
bool tablesEqual(struct router *table1, struct router table2) {
	for (int idx=0; idx<NUMROUTERS; idx++) {
		if (
			(table1->otherRouters[idx] != table2.otherRouters[idx])
			|| (table1->costs[idx] != table2.costs[idx])
			|| (table1->outgoingPorts[idx] != table2.outgoingPorts[idx])
			|| (table1->destinationPorts[idx] != table2.destinationPorts[idx]) )
			return false;
	}
	return true;
}

void outputTable(struct router *table) {
	FILE *f = NULL;
	switch (table->index) {
		case INDEXA:
			f = fopen("routing-outputA.txt", "a");
			break;
		case INDEXB:
			f = fopen("routing-outputB.txt", "a");
			break;
		case INDEXC:
			f = fopen("routing-outputC.txt", "a");
			break;
		case INDEXD:
			f = fopen("routing-outputD.txt", "a");
			break;
		case INDEXE:
			f = fopen("routing-outputE.txt", "a");
			break;
		case INDEXF:
			f = fopen("routing-outputF.txt", "a");
			break;
	}
	time_t ltime;
    ltime=time(NULL);
    fprintf(f, "\nTimestamp: %s\n",asctime( localtime(&ltime) ) );

    fprintf(f, "Destination, Cost, Outgoing Port, Destination Port\n");
    for (int i=0; i<NUMROUTERS; i++) {
    	fprintf(f, "%c %i %i %i\n",
    		table->otherRouters[i],
    		table->costs[i],
    		table->outgoingPorts[i],
    		table->destinationPorts[i]);
    }
    return;
}

/* updates table if possible. if table is changed, output to file */
void updateTable(struct router *currTable, struct router rcvdTable)
{
	bool isChanged = false;
	for (int i=0; i<NUMROUTERS; i++) {
		// ignore own entry in table
		if (i != currTable->index) {
			// find shortest paths to other routers
			if (rcvdTable.costs[i] == INT_MAX) {
				continue;
			} else if ( currTable->costs[i] > rcvdTable.costs[i] + currTable->costs[rcvdTable.index] ) {
				currTable->costs[i] = rcvdTable.costs[i] + currTable->costs[rcvdTable.index];
				currTable->outgoingPorts[i] = rcvdTable.index + 10000;
				currTable->destinationPorts[i] = rcvdTable.destinationPorts[i];
				isChanged = true;
			}
		}
	}
	if (isChanged)
		outputTable(currTable);
	return;
}

void initializeOutputFiles(struct router *network) {
	char routerLetters[] = "ABCDEF";
	char *s;
	int tableIndex = 0;
	for ( s = &routerLetters[0]; *s != '\0'; s++, tableIndex++ ) {
		char path[20];
		strcpy(path, "routing-output");
		size_t len = strlen(path);
		path[len] = *s;
		path[len+1] = '\0';
		strcat(path, ".txt");

		FILE *f = fopen(path, "w");
		if (f == NULL)
			error("Error opening file");
		/* Timestamp */
		time_t ltime;
	    ltime=time(NULL);
	    fprintf(f, "Timestamp: %s\n",asctime( localtime(&ltime) ) );
		
		fprintf(f, "Destination, Cost, Outgoing Port, Destination Port\n");
	    for (int i=0; i<NUMROUTERS; i++) {
	    	fprintf(f, "%c %i %i %i\n",
	    		network[tableIndex].otherRouters[i],
	    		network[tableIndex].costs[i],
	    		network[tableIndex].outgoingPorts[i],
	    		network[tableIndex].destinationPorts[i]);
	    }

		fclose(f);
	}
	return;
}

void initializeFromFile(struct router *tableA, struct router *tableB, struct router *tableC, struct router *tableD, struct router *tableE, struct router *tableF) {
	FILE *f = fopen("sample.txt", "r");
	if (f == NULL)
		error("Error opening sample file");
	char line[12];
	while (fgets(line, 12, f) != NULL) {
		char *ptr = strtok(line, ",");
		if (strcmp(ptr, "A") == 0) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			switch (atoi(ptrOP)) {
				case 10001:
					tableA->otherRouters[atoi(ptrOP)-10000] = 'B';
					break;
				case 10002:
					tableA->otherRouters[atoi(ptrOP)-10000] = 'C';
					break;
				case 10003:
					tableA->otherRouters[atoi(ptrOP)-10000] = 'D';
					break;
				case 10004:
					tableA->otherRouters[atoi(ptrOP)-10000] = 'E';
					break;
				case 10005:
					tableA->otherRouters[atoi(ptrOP)-10000] = 'F';
					break;
			}
			tableA->costs[atoi(ptrOP)-10000] = atoi(ptrCost);
			tableA->outgoingPorts[atoi(ptrOP)-10000] = ROUTERA;
			tableA->destinationPorts[atoi(ptrOP)-10000] = atoi(ptrOP);
		} else if (strcmp(ptr, "B") == 0) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			switch (atoi(ptrOP)) {
				case 10000:
					tableB->otherRouters[atoi(ptrOP)-10000] = 'A';
					break;
				case 10002:
					tableB->otherRouters[atoi(ptrOP)-10000] = 'C';
					break;
				case 10003:
					tableB->otherRouters[atoi(ptrOP)-10000] = 'D';
					break;
				case 10004:
					tableB->otherRouters[atoi(ptrOP)-10000] = 'E';
					break;
				case 10005:
					tableB->otherRouters[atoi(ptrOP)-10000] = 'F';
					break;
			}
			tableB->costs[atoi(ptrOP)-10000] = atoi(ptrCost);
			tableB->outgoingPorts[atoi(ptrOP)-10000] = ROUTERB;
			tableB->destinationPorts[atoi(ptrOP)-10000] = atoi(ptrOP);		
		} else if (strcmp(ptr, "C") == 0) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			switch (atoi(ptrOP)) {
				case 10000:
					tableC->otherRouters[atoi(ptrOP)-10000] = 'A';
					break;
				case 10001:
					tableC->otherRouters[atoi(ptrOP)-10000] = 'B';
					break;
				case 10003:
					tableC->otherRouters[atoi(ptrOP)-10000] = 'D';
					break;
				case 10004:
					tableC->otherRouters[atoi(ptrOP)-10000] = 'E';
					break;
				case 10005:
					tableC->otherRouters[atoi(ptrOP)-10000] = 'F';
					break;
			}
			tableC->costs[atoi(ptrOP)-10000] = atoi(ptrCost);
			tableC->outgoingPorts[atoi(ptrOP)-10000] = ROUTERC;
			tableC->destinationPorts[atoi(ptrOP)-10000] = atoi(ptrOP);
		} else if (strcmp(ptr, "D") == 0) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			switch (atoi(ptrOP)) {
				case 10000:
					tableD->otherRouters[atoi(ptrOP)-10000] = 'A';
					break;
				case 10001:
					tableD->otherRouters[atoi(ptrOP)-10000] = 'B';
					break;
				case 10002:
					tableD->otherRouters[atoi(ptrOP)-10000] = 'C';
					break;
				case 10004:
					tableD->otherRouters[atoi(ptrOP)-10000] = 'E';
					break;
				case 10005:
					tableD->otherRouters[atoi(ptrOP)-10000] = 'F';
					break;
			}
			tableD->costs[atoi(ptrOP)-10000] = atoi(ptrCost);
			tableD->outgoingPorts[atoi(ptrOP)-10000] = ROUTERD;
			tableD->destinationPorts[atoi(ptrOP)-10000] = atoi(ptrOP);
		} else if (strcmp(ptr, "E") == 0) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			switch (atoi(ptrOP)) {
				case 10000:
					tableE->otherRouters[atoi(ptrOP)-10000] = 'A';
					break;
				case 10001:
					tableE->otherRouters[atoi(ptrOP)-10000] = 'B';
					break;
				case 10002:
					tableE->otherRouters[atoi(ptrOP)-10000] = 'C';
					break;
				case 10003:
					tableE->otherRouters[atoi(ptrOP)-10000] = 'D';
					break;
				case 10005:
					tableE->otherRouters[atoi(ptrOP)-10000] = 'F';
					break;
			}
			tableE->costs[atoi(ptrOP)-10000] = atoi(ptrCost);
			tableE->outgoingPorts[atoi(ptrOP)-10000] = ROUTERE;
			tableE->destinationPorts[atoi(ptrOP)-10000] = atoi(ptrOP);
		} else if (strcmp(ptr, "F") == 0) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			switch (atoi(ptrOP)) {
				case 10000:
					tableF->otherRouters[atoi(ptrOP)-10000] = 'A';
					break;
				case 10001:
					tableF->otherRouters[atoi(ptrOP)-10000] = 'B';
					break;
				case 10002:
					tableF->otherRouters[atoi(ptrOP)-10000] = 'C';
					break;
				case 10003:
					tableF->otherRouters[atoi(ptrOP)-10000] = 'D';
					break;
				case 10004:
					tableF->otherRouters[atoi(ptrOP)-10000] = 'E';
					break;
			}
			tableF->costs[atoi(ptrOP)-10000] = atoi(ptrCost);
			tableF->outgoingPorts[atoi(ptrOP)-10000] = ROUTERF;
			tableF->destinationPorts[atoi(ptrOP)-10000] = atoi(ptrOP);
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

	/* for development */
	// struct router tableA = {
	// 	INDEXA,
	// 	{   'A', 	 'B', 	 'C',     'D',     'E',      'F'   },
	// 	{    0, 	  3, 	INT_MAX, INT_MAX,   1, 	   INT_MAX },
	// 	{ ROUTERA, ROUTERA,  NULL,    NULL,   ROUTERA,  NULL   },
	// 	{ ROUTERA, ROUTERB,  NULL,    NULL,   ROUTERE,  NULL   }
	// };

	// struct router tableB = {
	// 	INDEXB,
	// 	{   'A', 	 'B', 	  'C', 	   'D',	   'E',     'F'    },
	// 	{ 	 3, 	  0, 	   3, 	 INT_MAX, 	2, 	     1     },
	// 	{ ROUTERB, ROUTERB, ROUTERB,  NULL,   ROUTERB, ROUTERB },
	// 	{ ROUTERA, ROUTERB, ROUTERC,  NULL,   ROUTERE, ROUTERF }
	// };

	// struct router tableC = {
	// 	INDEXC,
	// 	{   'A', 	 'B', 	  'C', 	   'D',	   'E', 	 'F'   },
	// 	{ INT_MAX,    3, 	   0, 	    2, 	  INT_MAX, 	  1    },
	// 	{   NULL,  ROUTERC, ROUTERC, ROUTERC,  NULL,   ROUTERC },
	// 	{   NULL,  ROUTERB, ROUTERC, ROUTERD,  NULL,   ROUTERF }
	// };

	// struct router tableD = {
	// 	INDEXD,
	// 	{   'A',    'B',      'C',    'D',	   'E',     'F'   },
	// 	{ INT_MAX, INT_MAX,    2, 	   0, 	  INT_MAX, 	 3    },
	// 	{   NULL,   NULL,   ROUTERD, ROUTERD,  NULL,  ROUTERD },
	// 	{   NULL,   NULL,   ROUTERC, ROUTERD,  NULL,  ROUTERF }
	// };

	// struct router tableE = {
	// 	INDEXE,
	// 	{  'A',      'B',     'C',     'D',     'E',     'F'   },
	// 	{   1,        2,    INT_MAX, INT_MAX,    0,       3    },
	// 	{ ROUTERE, ROUTERE,   NULL,    NULL,  ROUTERE, ROUTERE },
	// 	{ ROUTERA, ROUTERB,   NULL,    NULL,  ROUTERE, ROUTERF }
	// };

	// struct router tableF = {
	// 	INDEXF,
	// 	{  'A',       'B',     'C',    'D',     'E',     'F'    },
	// 	{  INT_MAX,    1,       1,      3,       3,       0     },
	// 	{   NULL,   ROUTERF, ROUTERF, ROUTERF, ROUTERF, ROUTERF },
	// 	{   NULL,   ROUTERB, ROUTERC, ROUTERD, ROUTERE, ROUTERF }
	// };

	// for (int i=0; i<NUMROUTERS; i++) {
	// 	if (tableA.outgoingPorts[i] == ROUTERA)
	// 		tableA.neighbors += tableA.otherRouters[i];
	// 	if (tableB.outgoingPorts[i] == ROUTERB)
	// 		tableB.neighbors += tableB.otherRouters[i];
	// 	if (tableC.outgoingPorts[i] == ROUTERC)
	// 		tableC.neighbors += tableC.otherRouters[i];
	// 	if (tableD.outgoingPorts[i] == ROUTERD)
	// 		tableD.neighbors += tableD.otherRouters[i];
	// 	if (tableE.outgoingPorts[i] == ROUTERE)
	// 		tableE.neighbors += tableE.otherRouters[i];
	// 	if (tableF.outgoingPorts[i] == ROUTERF)
	// 		tableF.neighbors += tableF.otherRouters[i];
	// }

	/* end development */


	/* for testing */

	struct router tableA = {
		INDEXA,
		{   'A',     NULL,    NULL,    NULL,    NULL,    NULL   },
		{    0,     INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX },
		{ ROUTERA,   NULL,    NULL,    NULL,    NULL,    NULL   },
		{ ROUTERA,     NULL,    NULL,    NULL,    NULL,    NULL   }
	};
	struct router tableB = {
		INDEXB,
		{  NULL,     'B',    NULL,    NULL,    NULL,    NULL    },
		{ INT_MAX,    0,    INT_MAX, INT_MAX, INT_MAX, INT_MAX  },
		{  NULL,   ROUTERB,   NULL,    NULL,    NULL,    NULL   },
		{  NULL,   ROUTERB,   NULL,    NULL,    NULL,    NULL   }
	};
	struct router tableC = {
		INDEXC,
		{  NULL,     NULL,     'C',      NULL,    NULL,    NULL   },
		{ INT_MAX, INT_MAX,     0,     INT_MAX, INT_MAX, INT_MAX  },
		{  NULL,     NULL,   ROUTERC,    NULL,    NULL,    NULL   },
		{  NULL,     NULL,   ROUTERC,    NULL,    NULL,    NULL   }
	};
	struct router tableD = {
		INDEXD,
		{  NULL,     NULL,    NULL,    'D',     NULL,    NULL    },
		{ INT_MAX, INT_MAX, INT_MAX,    0,    INT_MAX, INT_MAX   },
		{  NULL,     NULL,    NULL,  ROUTERD,    NULL,    NULL   },
		{  NULL,     NULL,    NULL,  ROUTERD,    NULL,    NULL   }
	};
	struct router tableE = {
		INDEXE,
		{  NULL,     NULL,    NULL,    NULL,    'E',    NULL     },
		{ INT_MAX, INT_MAX, INT_MAX, INT_MAX,    0,    INT_MAX   },
		{  NULL,     NULL,    NULL,    NULL,  ROUTERE,    NULL   },
		{  NULL,     NULL,    NULL,    NULL,  ROUTERE,    NULL   }
	};
	struct router tableF = {
		INDEXF,
		{  NULL,     NULL,    NULL,    NULL,    NULL,     'F'   },
		{ INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX,    0     },
		{  NULL,     NULL,    NULL,    NULL,    NULL,  ROUTERF  },
		{  NULL,     NULL,    NULL,    NULL,    NULL,  ROUTERF  }
	};

	initializeFromFile(&tableA, &tableB, &tableC, &tableD, &tableE, &tableF);	

	/* end testing */

	// struct router *network = malloc(NUMROUTERS * sizeof(struct router));
	// network[0] = tableA;
	// network[1] = tableB;
	// network[2] = tableC;
	// network[3] = tableD;
	// network[4] = tableE;
	// network[5] = tableF;

	// initializeOutputFiles(network);

	// for (int i=0; i<2; i++) {
	// 	/* create parent socket */
	// 	if ( (sockfd[i] = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
	// 		error("Error opening socket");

	// 	/* server can be rerun immediately after killed */
	// 	optval = 1;
	// 	setsockopt(sockfd[i], SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

	// 	/* build server's Internet address */
	// 	bzero((char *) &serveraddr[i], sizeof(serveraddr[i]));
	// 	serveraddr[i].sin_family = AF_INET;
	// 	serveraddr[i].sin_addr.s_addr = htonl(INADDR_ANY);

	// 	switch(i+10000) {
	// 		case ROUTERA:
	// 			serveraddr[i].sin_port = htons(ROUTERA);
	// 			break;
	// 		case ROUTERB:
	// 			serveraddr[i].sin_port = htons(ROUTERB);
	// 			break;
	// 	}
	// 	/* bind: associate parent socket with port */
	// 	if (bind(sockfd[i], (struct sockaddr *) &serveraddr[i], sizeof(serveraddr[i])) < 0)
	// 		error("Error on binding");
	// }

	// clientlen = sizeof(clientaddr);

	// int nsocks = max(sockfd[0], sockfd[1]) + 1;

	// int test=1;
	// /* loop: wait for datagram, then echo it */
	// while (test) {
	// 	FD_ZERO(&socks);
	// 	FD_SET(sockfd[0], &socks);
	// 	FD_SET(sockfd[1], &socks);

	// 	if (select(nsocks, &socks, NULL, NULL, NULL) < 0) {
	// 		printf("Error selecting socket\n");
	// 	} else {
	// 		/* receives UDP datagram from client */
	// 		memset(buf, 0, BUFSIZE);
	// 		if (FD_ISSET(sockfd[0], &socks)) {
	// 			if ( (n = recvfrom(sockfd[0], buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
	// 				error("Error receiving datagram from client\n");
	// 			// memset(&buf, 0, BUFSIZE);
	// 			// memcpy(buf, &tableA, sizeof(struct router));
				
	// 			/* receives UDP datagram from client */
	// 			// hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
	// 			// if (hostp == NULL)
	// 			// 	error("Error getting host by address");
	// 			// /* Converts address from network bytes to IPv4 decimal notation string */
	// 			// hostaddrp = inet_ntoa(clientaddr.sin_addr);
	// 			// if (hostaddrp == NULL)
	// 			// 	error ("Error on ntoa");
	// 			// printf("Router received datagram from Port %d. Datagram sent to Port %d\n", ntohs(clientaddr.sin_port), ntohs(serveraddr[0].sin_port));
	// 			// printf("Buffer contains %d bytes: %s\n", sizeof(router), buf);

	// 			struct router *compTable = NULL;
	// 			// bzero(compTable, BUFSIZE);
	// 			compTable = malloc(BUFSIZE);
	// 			memcpy(compTable, buf, sizeof(struct router));

	// 			updateTable(&tableA, *compTable);
	// 			free(compTable);

	// 			memset(buf, 0, BUFSIZE);
	// 			memcpy(buf, &tableA.index, sizeof(tableA.index));
	// 			memcpy(buf + sizeof(tableA.index), &tableA.otherRouters, sizeof(tableA.otherRouters));
	// 			memcpy(buf + sizeof(tableA.index) + sizeof(tableA.otherRouters), &tableA.costs, sizeof(tableA.costs));
	// 			memcpy(buf + sizeof(tableA.index) + sizeof(tableA.otherRouters) + sizeof(tableA.costs), &tableA.outgoingPorts, sizeof(tableA.outgoingPorts));
	// 			memcpy(buf + sizeof(tableA.index) + sizeof(tableA.otherRouters) + sizeof(tableA.costs) + sizeof(tableA.outgoingPorts), &tableA.destinationPorts, sizeof(tableA.destinationPorts));
	// 			/* neighbors determined by outgoing port being the same port */
	// 			// for (int idx=0; idx<NUMROUTERS; idx++) {
	// 			// 	if (tableA.outgoingPorts[idx] == ROUTERA && idx != tableA.index) {
	// 			// 		/* send to neighbors */
	// 			// 		n = sendto(sockfd[0], buf, sizeof(struct router), 0, (struct sockaddr *)&serveraddr[1], clientlen);
	// 			// 		if (n < 0)
	// 			// 			error("Error sending to client");
	// 			// 	}
	// 			// }


	// 			n = sendto(sockfd[0], buf, sizeof(struct router), 0, (struct sockaddr *)&serveraddr[1], clientlen);

	// 			break;
	// 		} else if (FD_ISSET(sockfd[1], &socks)) {
	// 			if ( (n = recvfrom(sockfd[1], buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
	// 				error("Error receiving datagram from client\n");
	// 			// hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
	// 			// if (hostp == NULL)
	// 			// 	error("Error getting host by address");
	// 			// /* Converts address from network bytes to IPv4 decimal notation string */
	// 			// hostaddrp = inet_ntoa(clientaddr.sin_addr);
	// 			// if (hostaddrp == NULL)
	// 			// 	error ("Error on ntoa");
	// 			// printf("Router received datagram from Port %d. Datagram sent to Port %d\n", ntohs(clientaddr.sin_port), ntohs(serveraddr[1].sin_port));
	// 			// printf("Router received %d/%d bytes: %s\n", strlen(buf), n, buf);

	// 			memset(&buf, 0, BUFSIZE);
	// 			memcpy(buf, &tableB, sizeof(struct router));

	// 			/* echo input back to client */
	// 			n = sendto(sockfd[1], buf, sizeof(struct router), 0, (struct sockaddr *)&serveraddr[0], clientlen);
	// 			if (n < 0)
	// 				error("Error sending to client");
	// 		}
	// 		if (stableState()) {
	// 			break;
	// 		}
	// 	}
	// }
}