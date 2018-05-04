all: server client

server: server.c shared.h
	gcc -g -Wall -o server server.c shared.h -pthread -lrt

client: client.c shared.h
	gcc -g -Wall -o client client.c shared.h -pthread -lrt

clean:
	/bin/rm -fr *~ *.o client server
