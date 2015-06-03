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

// struct node is size 84
struct router
{
	int index;
	char otherRouters[NUMROUTERS];
	int costs[NUMROUTERS];
	int outgoingPorts[NUMROUTERS];
	int destinationPorts[NUMROUTERS];
};

/* printRouter()
 *
 * Prints out the given router table.
 */
void printRouter(struct router* r)
{
        printf("*** ROUTER INFO ***\nIndex: %d\n", r->index);
        printf("otherRouters\tcosts\t\toutgoingPorts\tdestPorts\n");
        int i;
        for (i=0; i < NUMROUTERS; i++)
                printf("%c\t\t%d\t\t%d\t\t%d\n", r->otherRouters[i], r->costs[i], r->outgoingPorts[i], r->destinationPorts[i]);
        printf("*** END RTRINFO ***\n");
}

/* printBuffer()
 *
 * Prints out the given int buffer.
 */
void printBuffer(int * buf)
{
	int i;
	for (i = 0; i < BUFSIZE; i++)
		printf("%d ", buf[i]);
	printf("\n");
}

struct matrix
{
	int r[NUMROUTERS][NUMROUTERS];
};

void error(char *msg) {
	perror(msg);
	exit(1);
}

/* getTime()
 *
 * Returns a string containing the time down to the milliseconds
 */
char* getTime() {
	struct timeval tval;
	struct tm* ptm;
	char time_string[40];
	long milliseconds;
	char t[50];

	gettimeofday(&tval, NULL);
	ptm = localtime(&tval.tv_sec);
	strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", ptm);
	milliseconds = tval.tv_usec/1000;
	sprintf(t, "%s.%03ld\n", time_string, milliseconds);
	return t;
}

/* tableToBuffer()
 *
 * Converts a router struct representation into an int buffer representation of a router.
 */
void tableToBuffer(struct router *table, int *buf) {
	
	int offset = 0;
	buf[offset] = table->index;
	offset++;

	int i;
	for (i = 0; i < NUMROUTERS; i++)
	{
		buf[offset + 0*NUMROUTERS] = (int) table->otherRouters[i];
		buf[offset + 1*NUMROUTERS] = table->costs[i];
		buf[offset + 2*NUMROUTERS] = table->outgoingPorts[i];
		buf[offset + 3*NUMROUTERS] = table->destinationPorts[i];
		offset++;
	}
}

/* bufferToTable()
 *
 * Converts an integer array representation into a struct representation of a router.
 */
void bufferToTable(int *buf, struct router *table) {

	int offset = 0;
	table->index = buf[offset];
	offset++;

	int i;
	for (i = 0; i < NUMROUTERS; i++)
	{
		table->otherRouters[i] = (char) buf[offset + 0*NUMROUTERS];
		table->costs[i] =		buf[offset + 1*NUMROUTERS];
		table->outgoingPorts[i] =	buf[offset + 2*NUMROUTERS];
		table->destinationPorts[i] =	buf[offset + 3*NUMROUTERS];
		offset++;
	}
}

/* outputTable()
 *
 * Writes the routing table to its output file.
 */
void outputTable(struct router *table, bool isStable) {
	FILE *f = NULL;
        int timeBufferSize = 64;
        char timeBuffer[timeBufferSize];

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

	if (!isStable) {
	    time_t ltime;
	    struct tm* timeinfo;

	    time(&ltime);
	    timeinfo = localtime(&ltime);
	    snprintf(timeBuffer, timeBufferSize, "%ld", timeinfo);
	    fprintf(f, "\nTimestamp: %s\nDestination, Cost, Outgoing Port, Destination Port\n", timeBuffer);
	} else {
		fprintf(f, "\nTable in Stable State\nDestination, Cost, Outgoing Port, Destination Port\n");
	}

	int i;
    for (i=0; i<NUMROUTERS; i++) {
    	fprintf(f, "%c %i %i %i\n",
    		table->otherRouters[i],
    		table->costs[i],
    		table->outgoingPorts[i],
    		table->destinationPorts[i]);
    }
    return;
}

/* updateTable()
 *
 * Updates table if possible. If table is changed, output to file.
 */
bool updateTable(struct router *currTable, struct router rcvdTable) {
	bool isChanged = false;
	int i;
	for (i=0; i<NUMROUTERS; i++) {
		// ignore own entry in table
		if (i != currTable->index) {
			// find shortest paths to other routers
			if (rcvdTable.costs[i] == INT_MAX) {
				continue;
			} else if ( currTable->costs[i] > rcvdTable.costs[i] + currTable->costs[rcvdTable.index] ) {

				currTable->otherRouters[i] = rcvdTable.otherRouters[i];
				currTable->costs[i] = rcvdTable.costs[i] + currTable->costs[rcvdTable.index];
				currTable->outgoingPorts[i] = rcvdTable.index + 10000;
				currTable->destinationPorts[i] = rcvdTable.destinationPorts[i];

				isChanged = true;
			}
		}
	}
	if (isChanged) {
		outputTable(currTable, false);
	}
	return isChanged;
}

/* initializeOutputFiles()
 *
 * Initializes the routing-outputX.txt files from the routing tables.
 */
void initializeOutputFiles(struct router **network) {
		char routerLetters[NUMROUTERS] = "ABCDEF";

    	int tableIndex = 0, timeBufferSize = 64, writeBufferSize = 1024;

    	char timeBuffer[timeBufferSize];
		char writeBuffer[writeBufferSize];

        for ( ; tableIndex < 6; tableIndex++ ) {
            char path[20];
            snprintf(path, sizeof(path), "routing-output%c%s\0", routerLetters[tableIndex], ".txt");

            FILE *f = fopen(path, "w");

            if (f == NULL)
                error("Error opening file");
            // Timestamp
            time_t ltime;
		    struct tm* timeinfo;

		    time(&ltime);
		    timeinfo = localtime(&ltime);

            snprintf(timeBuffer, timeBufferSize, "%ld", timeinfo);

            fprintf(f, "Timestamp: %s\nDestination, Cost, Outgoing Port, Destination Port\n", timeBuffer);
            int i;
            for (i=0; i<NUMROUTERS; i++) {
                fprintf(f, "%c %i %i %i\n",
                    network[tableIndex]->otherRouters[i],
                    network[tableIndex]->costs[i],
                    network[tableIndex]->outgoingPorts[i],
                    network[tableIndex]->destinationPorts[i]);
            }
            fclose(f);
        }
        return;
}

/* initializeFromFile()
 *
 * Initializes the routing tables from the information in 'sample.txt'.
 */
struct matrix initializeFromFile(struct router *tableA, struct router *tableB, struct router *tableC, struct router *tableD, struct router *tableE, struct router *tableF) {
	FILE *f = fopen("sample.txt", "r");
	if (f == NULL)
		error("Error opening sample file");

	int numNeighbors[NUMROUTERS] = {0};

	struct matrix neighborMatrix;
	memset(neighborMatrix.r, -1, sizeof(int) * NUMROUTERS * NUMROUTERS);

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
					numNeighbors[0]++;
					break;
				case 10002:
					tableA->otherRouters[atoi(ptrOP)-10000] = 'C';
					numNeighbors[0]++;
					break;
				case 10003:
					tableA->otherRouters[atoi(ptrOP)-10000] = 'D';
					numNeighbors[0]++;
					break;
				case 10004:
					tableA->otherRouters[atoi(ptrOP)-10000] = 'E';
					numNeighbors[0]++;
					break;
				case 10005:
					tableA->otherRouters[atoi(ptrOP)-10000] = 'F';
					numNeighbors[0]++;
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
					numNeighbors[1]++;
					break;
				case 10002:
					tableB->otherRouters[atoi(ptrOP)-10000] = 'C';
					numNeighbors[1]++;
					break;
				case 10003:
					tableB->otherRouters[atoi(ptrOP)-10000] = 'D';
					numNeighbors[1]++;
					break;
				case 10004:
					tableB->otherRouters[atoi(ptrOP)-10000] = 'E';
					numNeighbors[1]++;
					break;
				case 10005:
					tableB->otherRouters[atoi(ptrOP)-10000] = 'F';
					numNeighbors[1]++;
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
					numNeighbors[2]++;
					break;
				case 10001:
					tableC->otherRouters[atoi(ptrOP)-10000] = 'B';
					numNeighbors[2]++;
					break;
				case 10003:
					tableC->otherRouters[atoi(ptrOP)-10000] = 'D';
					numNeighbors[2]++;
					break;
				case 10004:
					tableC->otherRouters[atoi(ptrOP)-10000] = 'E';
					numNeighbors[2]++;
					break;
				case 10005:
					tableC->otherRouters[atoi(ptrOP)-10000] = 'F';
					numNeighbors[2]++;
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
					numNeighbors[3]++;
					break;
				case 10001:
					tableD->otherRouters[atoi(ptrOP)-10000] = 'B';
					numNeighbors[3]++;
					break;
				case 10002:
					tableD->otherRouters[atoi(ptrOP)-10000] = 'C';
					numNeighbors[3]++;
					break;
				case 10004:
					tableD->otherRouters[atoi(ptrOP)-10000] = 'E';
					numNeighbors[3]++;
					break;
				case 10005:
					tableD->otherRouters[atoi(ptrOP)-10000] = 'F';
					numNeighbors[3]++;
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
					numNeighbors[4]++;
					break;
				case 10001:
					tableE->otherRouters[atoi(ptrOP)-10000] = 'B';
					numNeighbors[4]++;
					break;
				case 10002:
					tableE->otherRouters[atoi(ptrOP)-10000] = 'C';
					numNeighbors[4]++;
					break;
				case 10003:
					tableE->otherRouters[atoi(ptrOP)-10000] = 'D';
					numNeighbors[4]++;
					break;
				case 10005:
					tableE->otherRouters[atoi(ptrOP)-10000] = 'F';
					numNeighbors[4]++;
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
					numNeighbors[5]++;
					break;
				case 10001:
					tableF->otherRouters[atoi(ptrOP)-10000] = 'B';
					numNeighbors[5]++;
					break;
				case 10002:
					tableF->otherRouters[atoi(ptrOP)-10000] = 'C';
					numNeighbors[5]++;
					break;
				case 10003:
					tableF->otherRouters[atoi(ptrOP)-10000] = 'D';
					numNeighbors[5]++;
					break;
				case 10004:
					tableF->otherRouters[atoi(ptrOP)-10000] = 'E';
					numNeighbors[5]++;
					break;
			}
			tableF->costs[atoi(ptrOP)-10000] = atoi(ptrCost);
			tableF->outgoingPorts[atoi(ptrOP)-10000] = ROUTERF;
			tableF->destinationPorts[atoi(ptrOP)-10000] = atoi(ptrOP);
		}
	}

	int index=0;
	int i;
	for (i=0; i<NUMROUTERS; i++) {
		if (tableA->costs[i] != INT_MAX && tableA->costs[i] != 0) {
			neighborMatrix.r[0][index] = i;
			index++;
		}
	}
	index=0;
	i = 0;
	for (; i<NUMROUTERS; i++) {
		if (tableB->costs[i] != INT_MAX && tableB->costs[i] != 0) {
			neighborMatrix.r[1][index] = i;
			index++;
		}
	}
	i = 0;
	index=0;
	for (; i<NUMROUTERS; i++) {
		if (tableC->costs[i] != INT_MAX && tableC->costs[i] != 0) {
			neighborMatrix.r[2][index] = i;
			index++;
		}
	}
	index=0;
	i = 0;
	for (; i<NUMROUTERS; i++) {
		if (tableD->costs[i] != INT_MAX && tableD->costs[i] != 0) {
			neighborMatrix.r[3][index] = i;
			index++;
		}
	}
	i = 0;
	index=0;
	for (; i<NUMROUTERS; i++) {
		if (tableE->costs[i] != INT_MAX && tableE->costs[i] != 0) {
			neighborMatrix.r[4][index] = i;
			index++;
		}
	}
	index=0;
	i = 0;
	for (; i<NUMROUTERS; i++) {
		if (tableF->costs[i] != INT_MAX && tableF->costs[i] != 0) {
			neighborMatrix.r[5][index] = i;
			index++;
		}
	}
	
	return neighborMatrix;
}

int main(int argc, char *argv[])
{
	int sockfd[NUMROUTERS]; /* socket */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr[NUMROUTERS]; /* server's address */
	struct sockaddr_in clientaddr; /* client's address */
	struct hostent *hostp; /* client host info */
	int buf[BUFSIZE]; /* message buffer */
	char *hostaddrp; /* dotted decimal host address string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */
	int count;
	fd_set socks;
	bool stableState = false;

	/* for testing */

	struct router tableA = {
		INDEXA,
		{   'A',     NULL,    NULL,    NULL,    NULL,    NULL   },
		{    0,     INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX },
		{ ROUTERA,   NULL,    NULL,    NULL,    NULL,    NULL   },
		{ ROUTERA,   NULL,    NULL,    NULL,    NULL,    NULL   }
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

	struct matrix neighborMatrix = initializeFromFile(&tableA, &tableB, &tableC, &tableD, &tableE, &tableF);	
	/* end testing */

	struct router **network = malloc(NUMROUTERS * sizeof(struct router*));
	int i;
	for (i=0; i<NUMROUTERS; i++) {
		network[i] = malloc(NUMROUTERS * sizeof(struct router));
	}
	network[0] = &tableA;
	network[1] = &tableB;
	network[2] = &tableC;
	network[3] = &tableD;
	network[4] = &tableE;
	network[5] = &tableF;

	initializeOutputFiles(network);
	for (i=0; i<NUMROUTERS; i++) {
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
			case ROUTERC:
				serveraddr[i].sin_port = htons(ROUTERC);
				break;
			case ROUTERD:
				serveraddr[i].sin_port = htons(ROUTERD);
				break;
			case ROUTERE:
				serveraddr[i].sin_port = htons(ROUTERE);
				break;
			case ROUTERF:
				serveraddr[i].sin_port = htons(ROUTERF);
				break;
		}
		/* bind: associate parent socket with port */
		if (bind(sockfd[i], (struct sockaddr *) &serveraddr[i], sizeof(serveraddr[i])) < 0)
			error("Error on binding");
	}

	clientlen = sizeof(clientaddr);
	int serverlen = sizeof(serveraddr[0]);

	/* begin by having ROUTERA send DV to one neighbor */
	/* for this implementation, ROUTERA will send to ROUTERB */
	int start;
	for (i=0; i<NUMROUTERS; i++) {
		if (neighborMatrix.r[0][i] != -1) {
			start = neighborMatrix.r[0][i];
			break;
		}
	}
	/* in this case, start=1 */

	memset(buf, 0, BUFSIZE);
	tableToBuffer(&tableA, &buf);

	n = sendto(sockfd[0], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[start], clientlen);

	int nsocks = max(sockfd[0], sockfd[1]);
	for (i=2; i<NUMROUTERS; i++) {
		nsocks = max(nsocks, sockfd[i]);
	}
	int counterdd = 0;
	count = 0;
	/* loop: wait for datagram, then echo it */
	while (1) {
	//	count = 0;
		FD_ZERO(&socks);
		for (i=0; i<NUMROUTERS; i++) {
			FD_SET(sockfd[i], &socks);
		}
		
		if (select(nsocks+1, &socks, NULL, NULL, NULL) < 0) {
			printf("Error selecting socket\n");
		} else {
			/* receives UDP datagram from client */
			memset(buf, 0, BUFSIZE);
			if (FD_ISSET(sockfd[0], &socks)) {
				if ( (n = recvfrom(sockfd[0], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");

				struct router compTable;
				bufferToTable(buf, &compTable);

				// printf("Received table:\n");
				// printRouter(&compTable);
				// int l;
				// for (l = 0; l < NUMROUTERS; l++)
				// {
				// 	printf("%d\n", compTable.destinationPorts[l]);
				// }

				// printf("A's original table:\n");
				// printRouter(&tableA);

				if (updateTable(&tableA, compTable) == false)
					count++;
				else
					count = 0;

				// printf("A's updated table:\n");
				// printRouter(&tableA);

				tableToBuffer(&tableA, &buf);

				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[0][i] != -1) {
						n = sendto(sockfd[0], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[0][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
						// printf("Reached here A - and neighborMatrix[0][%i]: Value = %i\n", i, neighborMatrix.r[0][i]);
					}
				}
			}

			// else
			if (FD_ISSET(sockfd[1], &socks)) {
				if ( (n = recvfrom(sockfd[1], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
				{
					error("Error receiving datagram from client\n");
					printf("error\n");
				}

				struct router compTable;

				// printBuffer(buf);
				// printf("%d %d\n\n\n", sizeof(buf), sizeof(compTable));

				
				bufferToTable(&buf, &compTable);

				// printf("Received table:\n");
				// printRouter(&compTable);
				// printf("B's original table:\n");
				// printRouter(&tableB);

				if (updateTable(&tableB, compTable) == false)
					count++;
				else
					count = 0;

				// printf("B's updated table:\n");
				// printRouter(&tableB);

				tableToBuffer(&tableB, &buf);

				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[1][i] != -1) {
						n = sendto(sockfd[1], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[1][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
						//printf("Reached here B - and neighborMatrix[1][%i]: Value = %i\n", i, neighborMatrix.r[1][i]);
					}
				}
			}


			// else
			if (FD_ISSET(sockfd[2], &socks)) {
				if ( (n = recvfrom(sockfd[2], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");

				struct router compTable;

				bufferToTable(&buf, &compTable);

				// printf("Received table:\n");
				// printRouter(&compTable);
				// printf("C's original table:\n");
				// printRouter(&tableC);

				if(updateTable(&tableC, compTable) == false)
					count++;
				else
					count = 0;

				// printf("C's updated table:\n");
				// printRouter(&tableC);

				tableToBuffer(&tableC, &buf);

				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[2][i] != -1) {
						n = sendto(sockfd[2], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[2][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
						//printf("Reached here C - and neighborMatrix[2][%i]: Value = %i\n", i, neighborMatrix.r[2][i]);
					}
				}
			}
			//else
			if (FD_ISSET(sockfd[3], &socks)) {
				if ( (n = recvfrom(sockfd[3], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");

				struct router compTable;

				bufferToTable(&buf, &compTable);

				// printf("Received table:\n");
				// printRouter(&compTable);
				// printf("D's original table:\n");
				// printRouter(&tableD);

				if(updateTable(&tableD, compTable) == false)
					count++;
				else
					count = 0;

				// printf("D's updated table:\n");
				// printRouter(&tableD);

				tableToBuffer(&tableD, &buf);
				
				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[3][i] != -1) {
						n = sendto(sockfd[3], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[3][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
						//printf("Reached here D - and neighborMatrix[3][%i]: Value = %i\n", i, neighborMatrix.r[3][i]);
					}
				}
			}
			// else
			if (FD_ISSET(sockfd[4], &socks)) {
				if ( (n = recvfrom(sockfd[4], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");

				struct router compTable;

				bufferToTable(&buf, &compTable);

				// printf("Received table:\n");
				// printRouter(&compTable);
				// printf("E's original table:\n");
				// printRouter(&tableE);

				if(updateTable(&tableE, compTable) == false)
					count++;
				else
					count = 0;

				// printf("E's updated table:\n");
				// printRouter(&tableE);

				tableToBuffer(&tableE, &buf);

				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[4][i] != -1) {
						n = sendto(sockfd[4], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[4][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
						//printf("Reached here E - and neighborMatrix[4][%i]: Value = %i\n", i, neighborMatrix.r[4][i]);
					}
				}
			}
			// else
			if (FD_ISSET(sockfd[5], &socks)) {
				if ( (n = recvfrom(sockfd[5], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");

				struct router compTable;

				bufferToTable(&buf, &compTable);

				// printf("Received table:\n");
				// printRouter(&compTable);
				// printf("F's original table:\n");
				// printRouter(&tableF);

				if(updateTable(&tableF, compTable) == false)
					count++;
				else
					count = 0;

				// printf("F's updated table:\n");
				// printRouter(&tableF);

				tableToBuffer(&tableF, &buf);

				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[5][i] != -1) {
						n = sendto(sockfd[5], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[5][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
						//printf("Reached here F - and neighborMatrix[5][%i]: Value = %i\n", i, neighborMatrix.r[5][i]);
					}
				}
			}
			if (count >= NUMROUTERS * 2) {
				stableState = true;
				 printf("\n\nFINAL ROUTER INFO:\n\n");
	 printf("ROUTER A:\n\n");
	 printRouter(&tableA);
	 printf("\n\nROUTER B:\n\n");
	 printRouter(&tableB);
	 printf("\n\nROUTER C:\n\n");
	 printRouter(&tableC);
	 printf("\n\nROUTER D:\n\n");
	 printRouter(&tableD);
	 printf("\n\nROUTER E:\n\n");
	 printRouter(&tableE);
	 printf("\n\nROUTER F:\n\n");
	 printRouter(&tableF);
				printf("\n\nWhat do you want to do?\n\n1) kill router\n2) send packet\n\n");
				// steady state
				// scan for input to send a packe
				int option = 6; // kill router, or send packet from x to y
				char toKill = NULL;
				int srcIndex = NULL, dstIndex = NULL;
				scanf("%d", &option);
				switch (option)
				{
					case 1:
						printf("Type the router number to kill:\n");
						scanf("\n%c", &toKill);
						printf("Killing router %c\n", toKill);
						// set the link cost to the specified router to INT_MAX
						// for each router
							// send their new routing tables to each neighbor
						
						tableA.costs[toKill - 'a'] = INT_MAX;
						tableB.costs[toKill - 'a'] = INT_MAX;
						tableC.costs[toKill - 'a'] = INT_MAX;
						tableD.costs[toKill - 'a'] = INT_MAX;
						tableE.costs[toKill - 'a'] = INT_MAX;
						tableF.costs[toKill - 'a'] = INT_MAX;
						// resend all of the routers DVs to each neighbor
						for (i = 0; i < NUMROUTERS; i++)
						{
							if (toKill - 'a' == i)
								continue;
							// send dv to all of i's numbers
							// put ith table into a buffer and send to neighbors
							struct router * toSend = network[i];
							tableToBuffer(toSend, buf);
							int j;
							for (j = 0; j < NUMROUTERS; j++)
							{
								if (neighborMatrix.r[i][j] != -1)
								{
									// FLUSH the neighbors socket
//									while (0 < recvfrom(sockfd[], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)))
									n = sendto(sockfd[i], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[i][j]], clientlen);
									if (n < 0)
										error("Error sending to client");
								}
							}
						}

						break;
					case 2:
						printf("Type [SRC_ROUTER_PORT#] [DEST_ROUTER_PORT#]\n");
						scanf("%d %d", &srcIndex, &dstIndex);
						printf("Routing a packet from %d to %d\n", srcIndex, dstIndex);
						break;
				} 

			//	break;
			}

		}
	}
	for (i=0; i<NUMROUTERS; i++) {
		outputTable(network[i], true);
	}
}
