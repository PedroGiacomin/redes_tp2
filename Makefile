all:
	gcc -Wall -c common.c
	gcc -Wall device.c common.o -o device -pthread
	gcc -Wall server.c common.o -o server

clean:
	rm common.o device server server-mt
