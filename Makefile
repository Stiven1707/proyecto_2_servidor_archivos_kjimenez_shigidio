all: server.o client.o protocol.o split.o
	gcc -o server server.o protocol.o  -pthread
	gcc -o client client.o protocol.o split.o

debug: server_debug.o client_debug.o protocol_debug.o split_debug.o
	gcc -o server server_debug.o protocol_debug.o -pthread
	gcc -o client client_debug.o protocol_debug.o split_debug.o

server.o: server.c
	gcc -c -o server.o server.c

client.o: client.c
	gcc -c -o client.o client.c

protocol.o: protocol.c
	gcc -c -o protocol.o protocol.c

split.o: split.c
	gcc -c -o split.o split.c

server_debug.o: server.c 
	gcc -c -DDEBUG -g -c -o server_debug.o server.c

client_debug.o: client.c
	gcc -c -DDEBUG -g -c -o client_debug.o client.c

protocol_debug.o: protocol.c
	gcc -c -DDEBUG -g -c -o protocol_debug.o protocol.c

split_debug.o: split.c
	gcc -DDEBUG -g -c -o split_debug.o split.c



clean:
	rm -rf *.o server client files/ client_files/
