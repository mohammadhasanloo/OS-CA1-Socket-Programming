all: client.out server.out

client.out: Client.o
	gcc Client.o -o client.out

server.out: Server.o
	gcc Server.o -o server.out	

Client.o: Client.c ConstantValues.h
	gcc -c Client.c -o Client.o

Server.o: Server.c ConstantValues.h
	gcc -c Server.c -o Server.o


clean:
	rm -f *.out
	rm -r -f *.o