all: server

server: server.c
	gcc -w -o server server.c

clean:
	rm -f server routing-outputA.txt routing-outputB.txt routing-outputD.txt routing-outputE.txt routing-outputF.txt 
