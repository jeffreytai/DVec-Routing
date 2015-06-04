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
#include <ctype.h>

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

struct packet
{
	char flag;
	char message[50];
	char srcNode;
	char dstNode;
	int arrivalPort;
	int forwardingPort;
};

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
void printBuffer(int * buf, size_t size)
{
	int i;
	for (i = 0; i < size; i++)
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
	fclose(f);
    return;
}

/* getDestPortIndex()
 *
 * Returns the index of the portno in the table's destination array
 */
int getDestPortIndex(struct router* table, int portno) {
	int i;
	for (i=0; i<NUMROUTERS; i++) {
		if (table->destinationPorts[i] == portno)
			return i;
	}
}

/* routerToPort()
 *
 * Returns the Router PORTNO given the name
 */
int routerToPort(char r) {
	switch (r)
	{
		case 'a':
		case 'A':
			return ROUTERA;
			break;
		case 'b':
		case 'B':
			return ROUTERB;
			break;
		case 'c':
		case 'C':
			return ROUTERC;
			break;
		case 'd':
		case 'D':
			return ROUTERD;
			break;
		case 'e':
		case 'E':
			return ROUTERE;
			break;
		case 'f':
		case 'F':
			return ROUTERF;
			break;
	}
}

/* portToRouter()
 *
 * Returns the router name given the port
 */
char portToRouter(int portno) {
	switch (portno)
	{
		case ROUTERA:
			return 'A';
			break;
		case ROUTERB:
			return 'B';
			break;
		case ROUTERC:
			return 'C';
			break;
		case ROUTERD:
			return 'D';
			break;
		case ROUTERE:
			return 'E';
			break;
		case ROUTERF:
			return 'F';
			break;
	}
}

/* tableName()
 *
 * Returns the name of the table given the DV
 */
char tableName(struct router* table) {
	int i;
	for (i=0; i<NUMROUTERS; i++) {
		if (table->costs[i] == 0) {
			switch(i)
			{
				case 0:
					return 'A';
					break;
				case 1:
					return 'B';
					break;
				case 2:
					return 'C';
					break;
				case 3:
					return 'D';
					break;
				case 4:
					return 'E';
					break;
				case 5:
					return 'F';
					break;
			}
		}
	}
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

/* outputPacket()
 *
 * Writes packet info to the router output file 
 */
void outputPacket(struct router *table, struct packet *p, bool isDestination) {
	// write to output timestamp, src node, dst node, arrival UDP port, and outgoing UDP port
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

	time_t ltime;
	struct tm* timeinfo;

	time(&ltime);
	timeinfo = localtime(&ltime);
	snprintf(timeBuffer, timeBufferSize, "%ld", timeinfo);

	if (!isDestination) {
		int destIndex;
		char tname;

		destIndex = getDestPortIndex(table, routerToPort(p->dstNode));
		tname = tableName(table);

		fprintf(f, "\nReceived data packet:\nTimestamp: %s\nSource Node: %c\nDestination Node: %c\nArrival UDP Port: %i\nOutgoing UDP Port: %i\n", timeBuffer, p->srcNode, p->dstNode, routerToPort(tname), table->outgoingPorts[destIndex]);
	} else {
		fprintf(f, "\nCumulative information about data packet:\nTimestamp: %s\nMessage: %s\nSource Node: %c\nDestination Node: %c\nArrival (Destination) UDP Port: %i\n", timeBuffer, p->message, p->srcNode, p->dstNode, routerToPort(p->dstNode));
	}
}

/* routerToTable()
 *
 * Returns the DV given the router name
 */
struct router* routerToTable(struct router** network, char r) {
	switch(r)
	{
		case 'a':
		case 'A':
			return network[0];
			break;
		case 'b':
		case 'B':
			return network[1];
			break;
		case 'c':
		case 'C':
			return network[2];
			break;
		case 'd':
		case 'D':
			return network[3];
			break;
		case 'e':
		case 'E':
			return network[4];
			break;
		case 'f':
		case 'F':
			return network[5];
			break;
	}
}

/* forwardPacket()
 *
 * Forwards a packet from the source to destination
 */
void forwardPacket(struct packet *p, struct router** network) {
	char src = p->srcNode;
	char dst = p->dstNode;
	int index = 0;
	int i;

	int destIndex;
	int outgoing;
	char nextRouter;

	struct router* curr = malloc(NUMROUTERS * sizeof(struct router));
	curr = routerToTable(network, src);
	p->arrivalPort = routerToPort(tableName(curr));
	while (tableName(curr) != dst) {
		destIndex = getDestPortIndex(curr, routerToPort(dst));
		outgoing = curr->outgoingPorts[destIndex];
		p->forwardingPort = outgoing;
		outputPacket(curr, p, false);
		if (outgoing == routerToPort(tableName(curr))) {
			// next router is going to be destination router
			break;
		}

		nextRouter = portToRouter(outgoing);
		curr = routerToTable(network, nextRouter);
		index++;
	}

	// Reached destination router
	struct router* dstRouter = malloc(NUMROUTERS * sizeof(struct router));
	dstRouter = routerToTable(network, dst);
	outputPacket(dstRouter, p, true);

	return;

	// while current router is not destination router
		// get index of destination port
		// use index to get outgoing port
		// write to output timestamp, src node, dst node, arrival UDP port, and outgoing UDP port
		// assign arrival port and forwarding port
		// if outgoing port is same as current port (next router is dst router)
			// break;

	// at destination router...
		// output data (message)
		// output src and dst router
		// output each arrival and outgoing port that isn't equal to 0
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

/* reinitalizeTopologyFile
 *
 *
 */
void reinitializeTopologyFile(char killedRouter, int n)
{
	// read lines from topology file
	// if any line has the killed router in it, make the link cost INT_MAX

	FILE *f = fopen("sample.txt", "r");
	if (f == NULL)
		error("Error opening sample file");

	char line[32];
	char writebuf[512];
	int canary = 0;
	int linecount = 1;
	bool changeCost = false;

	while (fgets(line, 32, f) != NULL) {
		char *ptr = strtok(line, ",");

		if (ptr[0] == killedRouter) {
			changeCost = true;
			// change the path cost on this one
		}

		char *ptr2 = strtok(NULL, ",");

		if (ptr2[0] == killedRouter) {
			changeCost = true;
			// change the path cost on this one
		}
	
		char *ptrOP = strtok(NULL, ",");
		char *ptrCost = strtok(NULL, "\n");

		if (canary == 0)
		{
			strcpy(writebuf, ptr);
			canary++;
		}
		else
		{
			strcat(writebuf, ptr);
		}

		strcat(writebuf, ",");
		strcat(writebuf, ptr2);
		strcat(writebuf, ",");
		strcat(writebuf, ptrOP);
		strcat(writebuf, ",");
		strcat(writebuf, (changeCost ? "2147683647" : ptrCost));
		// if not end of file)
		if (linecount != 18) // number of lines in network topo file
		{
			strcat(writebuf, "\n");
		}
		changeCost = false;
		linecount++;
	}
	fclose(f);
	f = fopen("sample.txt", "w+");

	// overwrite the file with the new write buffer contents
	fprintf(f, "%s", writebuf);
	fclose(f);
}

void reinitializeTables(struct router *tableA, struct router *tableB, struct router *tableC, struct router *tableD, struct router *tableE, struct router *tableF) {
	int a;
	struct router *rp;
	int ind, val;
	for (a = 0; a < NUMROUTERS; a++)
	{		
		tableA->otherRouters[a] = NULL;
		tableB->otherRouters[a] = NULL;
		tableC->otherRouters[a] = NULL;
		tableD->otherRouters[a] = NULL;
		tableE->otherRouters[a] = NULL;
		tableF->otherRouters[a] = NULL;

		tableA->costs[a] = INT_MAX;
		tableB->costs[a] = INT_MAX;
		tableC->costs[a] = INT_MAX;
		tableD->costs[a] = INT_MAX;
		tableE->costs[a] = INT_MAX;
		tableF->costs[a] = INT_MAX;

		tableA->destinationPorts[a] = NULL;
		tableB->destinationPorts[a] = NULL;
		tableC->destinationPorts[a] = NULL;
		tableD->destinationPorts[a] = NULL;
		tableE->destinationPorts[a] = NULL;
		tableF->destinationPorts[a] = NULL;

		tableA->outgoingPorts[a] = NULL;
		tableB->outgoingPorts[a] = NULL;
		tableC->outgoingPorts[a] = NULL;
		tableD->outgoingPorts[a] = NULL;
		tableE->outgoingPorts[a] = NULL;
		tableF->outgoingPorts[a] = NULL;

		switch (a)
		{
			
			case 0:
				rp = tableA;	
				ind = INDEXA;
				val = ROUTERA;
				break;
			case 1:
				rp = tableB;
				ind = INDEXB;
				val = ROUTERB;
				break;
			case 2:
				rp = tableC;
				ind = INDEXC;
				val = ROUTERC;
				break;
			case 3:
				rp = tableD;
				ind = INDEXD;
				val = ROUTERD;
				break;
			case 4:
				rp = tableE;
				ind = INDEXE;
				val = ROUTERE;
				break;
			case 5:
				rp = tableF;
				ind = INDEXF;
				val = ROUTERF;
				break;
		}
		rp->index = ind;
		rp->otherRouters[a] = (char) ('A' + a);
		rp->costs[a] = 0;
		rp->destinationPorts[a] = val;
		rp->outgoingPorts[a] = val;
	}
}

/* initializeFromFile()
 *
 * Initializes the routing tables from the information in 'sample.txt'.
 */
struct matrix initializeFromFile(struct router *tableA, struct router *tableB, struct router *tableC, struct router *tableD, struct router *tableE, struct router *tableF, int killedRouters[]) {
	FILE *f = fopen("sample.txt", "r");
	if (f == NULL)
		error("Error opening sample file");

	int numNeighbors[NUMROUTERS] = {0};

	struct matrix neighborMatrix;
	memset(neighborMatrix.r, -1, sizeof(int) * NUMROUTERS * NUMROUTERS);

	char line[32];
	while (fgets(line, 32, f) != NULL) {
		char *ptr = strtok(line, ",");
		// printf("%s %s killed!\n", ptr, killedRouters[0] ? "is" : "is not");
		if (strcmp(ptr, "A") == 0 && !killedRouters[0]) {// and 'A' is not killed
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");

			// if ptrCost == INT_MAX
			//	
			// if ptr is killed, dont update the table
			//printf("%s %s killed!\n", ptr, killedRouters[ptr[0] - 'A'] ? "is" : "is not");
			if (!killedRouters[ptr[0] - 'A'])
			{
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
			}
		} else if (strcmp(ptr, "B") == 0 && !killedRouters[1]) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			// printf("\n\n%s\n\n", ptrCost);
			if (!killedRouters[ptr[0] - 'A'])
			{
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
			}
		} else if (strcmp(ptr, "C") == 0 && !killedRouters[2]) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			if (!killedRouters[ptr[0] - 'A'])
			{
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
			}
		} else if (strcmp(ptr, "D") == 0 && !killedRouters[3]) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			if (!killedRouters[ptr[0] - 'A'])
			{
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
			}
		} else if (strcmp(ptr, "E") == 0 && !killedRouters[4]) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			if (!killedRouters[ptr[0] - 'A'])
			{
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
			}
		} else if (strcmp(ptr, "F") == 0 && !killedRouters[5]) {
			ptr = strtok(NULL, ",");
			char *ptrOP = strtok(NULL, ",");
			char *ptrCost = strtok(NULL, ",");
			if (!killedRouters[ptr[0] - 'A'])
			{
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
int nKills = 0;
        char * filepath = "sample.txt";
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50000;
	int killedRouters[NUMROUTERS] = {0};

	//printBuffer(killedRouters, NUMROUTERS);
	/* for testing */
	struct router tableA, tableB, tableC, tableD, tableE, tableF;
/*
	struct router tableA = {
		INDEXA,
		{   'A',     NULL,    NULL,    NULL,    NULL,    NULL   },
		{    0,     INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX },
		{ ROUTERA,   NULL,    NULL,    NULL,    NULL,    NULL   },
		{ ROUTERA,   NULL,    NULL,    NULL,    NULL,    NULL   }
	};
*/
/*
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
*/
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
/*
	for(i=0;i < NUMROUTERS; i++)
	{
		printRouter(network[i]);
	}
*/
re_initialize:
/*
	if (nKills > 0)
	{
		printf("\nenter\n");
	}
*/
	reinitializeTables(&tableA, &tableB, &tableC, &tableD, &tableE, &tableF);
/*
	printf("***\n***AFTER REINIT:***\n***");	
	for(i=0;i < NUMROUTERS; i++)
	{
		printRouter(network[i]);
	}
*/
	struct matrix neighborMatrix = initializeFromFile(&tableA, &tableB, &tableC, &tableD, &tableE, &tableF, killedRouters);	
	/* end testing */
/*
	printf("***\n***SECOND PRINT:***\n***");	

	for(i=0;i < NUMROUTERS; i++)
	{
		printRouter(network[i]);
	}

	int g;
	for (i=0; i < NUMROUTERS; i++)
	{
		for (g=0; g < NUMROUTERS; g++)
			printf("%d\t", neighborMatrix.r[i][g]);	
		printf("\n");
	}
*/
	initializeOutputFiles(network);
	for (i=0; i<NUMROUTERS; i++) {
		/* create parent socket */
		if ( (sockfd[i] = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
			error("Error opening socket");

		/* server can be rerun immediately after killed */
		optval = 1;
		setsockopt(sockfd[i], SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
		setsockopt(sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

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
	int start;
	/* begin by having ROUTERA send DV to one neighbor */
	/* for this implementation, ROUTERA will send to ROUTERB */
	struct router* starter;
	switch (argv[1][0])
	{
		case 'A':
			starter = &tableA;
			start = INDEXA;
			break;
		case 'B':
			starter = &tableB;
			start = INDEXB;
			break;
		case 'C':
			starter = &tableC;
			start = INDEXC;
			break;
		case 'D':
			starter = &tableD;
			start = INDEXD;
			break;
		case 'E':
			starter = &tableE;
			start = INDEXE;
			break;
		case 'F':
			starter = &tableF;
			start = INDEXF;
			break;
	}
//exit(0);

/*

	for (i=0; i<NUMROUTERS; i++) {
		if (neighborMatrix.r[0][i] != -1) {
			start = neighborMatrix.r[0][i];
			break;
		}
	}
*/
// start is 0 thru 5
	//in this case, start=1
	//printRouter(starter);
	memset(buf, 0, BUFSIZE);
	tableToBuffer(starter, &buf);
//	tableToBuffer(&tableA, &buf);

//	printf("Starting router: %d %c\n", start - 'A', start);

//	n = sendto(sockfd[start - 'A'], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[start], clientlen);
	n = sendto(sockfd[start], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[start], clientlen);

	int nsocks = max(sockfd[0], sockfd[1]);
	for (i=2; i<NUMROUTERS; i++) {
		nsocks = max(nsocks, sockfd[i]);
	}
	int counterdd = 0;
	count = 0;
	printf("Stabilizing network...");
	/* loop: wait for datagram, then echo it */
	while (1) {
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

				if (updateTable(&tableA, compTable) == false)
					count++;
				else
					count = 0;

				tableToBuffer(&tableA, &buf);

				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[0][i] != -1) {
						n = sendto(sockfd[0], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[0][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
					}
				}
			}

			if (FD_ISSET(sockfd[1], &socks)) {
				if ( (n = recvfrom(sockfd[1], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
				{
					error("Error receiving datagram from client\n");
					printf("error\n");
				}

				struct router compTable;
				
				bufferToTable(&buf, &compTable);
				if (updateTable(&tableB, compTable) == false)
					count++;
				else
					count = 0;

				tableToBuffer(&tableB, &buf);

				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[1][i] != -1) {
						n = sendto(sockfd[1], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[1][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
					}
				}
			}

			if (FD_ISSET(sockfd[2], &socks)) {
				if ( (n = recvfrom(sockfd[2], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");

				struct router compTable;

				bufferToTable(&buf, &compTable);

				if(updateTable(&tableC, compTable) == false)
					count++;
				else
					count = 0;

				tableToBuffer(&tableC, &buf);

				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[2][i] != -1) {
						n = sendto(sockfd[2], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[2][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
					}
				}
			}
			if (FD_ISSET(sockfd[3], &socks)) {
				if ( (n = recvfrom(sockfd[3], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");

				struct router compTable;

				bufferToTable(&buf, &compTable);

				if(updateTable(&tableD, compTable) == false)
					count++;
				else
					count = 0;


				tableToBuffer(&tableD, &buf);
				
				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[3][i] != -1) {
						n = sendto(sockfd[3], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[3][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
					}
				}
			}
			if (FD_ISSET(sockfd[4], &socks)) {
				if ( (n = recvfrom(sockfd[4], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");

				struct router compTable;

				bufferToTable(&buf, &compTable);


				if(updateTable(&tableE, compTable) == false)
					count++;
				else
					count = 0;


				tableToBuffer(&tableE, &buf);

				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[4][i] != -1) {
						n = sendto(sockfd[4], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[4][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
					}
				}
			}
			if (FD_ISSET(sockfd[5], &socks)) {
				if ( (n = recvfrom(sockfd[5], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) < 0 )
					error("Error receiving datagram from client\n");

				struct router compTable;

				bufferToTable(&buf, &compTable);


				if(updateTable(&tableF, compTable) == false)
					count++;
				else
					count = 0;


				tableToBuffer(&tableF, &buf);

				for (i=0; i<NUMROUTERS; i++) {
					if (neighborMatrix.r[5][i] != -1) {
						n = sendto(sockfd[5], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&serveraddr[neighborMatrix.r[5][i]], clientlen);
						if (n < 0)
							error("Error sending to client");
					}
				}
			}
			if (count >= NUMROUTERS * 2) {
				for (i=0; i<NUMROUTERS; i++) {
					outputTable(network[i], true);
				}

				stableState = true;
				// printf("\n\nFINAL ROUTER INFO:\n\n");
				// printf("ROUTER A:\n\n");
				// printRouter(&tableA);
				// printf("\n\nROUTER B:\n\n");
				// printRouter(&tableB);
				// printf("\n\nROUTER C:\n\n");
				// printRouter(&tableC);
				// printf("\n\nROUTER D:\n\n");
				// printRouter(&tableD);
				// printf("\n\nROUTER E:\n\n");
				// printRouter(&tableE);
				// printf("\n\nROUTER F:\n\n");
				// printRouter(&tableF);
				printf("[OK]\n\n");
choose_action:
				printf("Press 1<ENTER> to kill a router\nPress 2<ENTER> to send packet across the network\n-> ");
				// steady state
				// scan for input to send a packe
				int option; // kill router, or send packet from x to y
				char toKill, srcRouter, dstRouter;
				scanf("%d", &option);
				switch (option)
				{
					case 1:
						printf("Label of router to kill (A-F):\n-> ");
						scanf("\n%c", &toKill);
						printf("Killing router %c\n", toupper(toKill));
						int k, n;
/*
                                		struct timeval tv;
                                		tv.tv_usec = 500000;
*/
                                		for (k = 0; k < NUMROUTERS; k++)
                                		{
							printf("Clearing Router %c's input buffers...", (char) k + 'A');
                                        		FD_ZERO(&socks);
                                        		FD_SET(sockfd[k], &socks);
                                                        while ( (n = recvfrom(sockfd[k], buf, BUFSIZE*sizeof(int), 0, (struct sockaddr *)&clientaddr, &clientlen)) > 0)
                                                        {
//                                                                printf("%d %d\n", k, n);
                                                        }
							printf("[OK]\n");
						}
						reinitializeTopologyFile(toupper(toKill), nKills);
						killedRouters[toupper(toKill) - 'A'] = 1;
						nKills++;
						goto re_initialize;

						// will never reach this point
						break;
					case 2:
						printf("Label of source router (A-F):\n-> ");
						scanf("%c", &srcRouter);
						srcRouter = getchar();
						srcRouter = toupper(srcRouter);
						printf("Label of destination router (A-F):\n-> ");
						scanf("%c", &dstRouter);
						dstRouter = getchar();
						dstRouter = toupper(dstRouter);
						printf("Routing a packet from Router %c to Router %c...", srcRouter, dstRouter);
						struct packet p = { 'd', "message", srcRouter, dstRouter, 0, 0 };
						forwardPacket(&p, network);
						printf("[OK]\n\n");
						goto choose_action;
						break;
				} 
				break;
			}
		}
	}
}
