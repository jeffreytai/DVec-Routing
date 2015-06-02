all: server client

server: server.c
	gcc -w -o server server.c

client: client.c
	gcc -w -o client client.c

clean:
	rm -f server client routing-outputA.txt routing-outputB.txt routing-outputD.txt routing-outputE.txt routing-outputF.txt 
