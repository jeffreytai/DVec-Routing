all: server client

server: server.c
	gcc -std=c99 -w -o server server.c

client: client.c
	gcc -w -o client client.c

clean:
	rm server client
