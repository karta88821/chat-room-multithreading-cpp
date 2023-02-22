CC=g++
#FLAGS=-O3 -Wall
LIBS=-pthread

# target: depandancy1, depandancy2, .... (if this target refer to .h file, put .h here, and don't need to put in the below command)
#     execute command

# output:  g++ -c output.out compiled1.o compiled2.o ...
# compile: g++ -c main.cpp

all: server client
server: server.o
	$(CC) $(LIBS) -o server server.o
client: client.o
	$(CC) $(LIBS) -o client client.o
server.o: server.cpp Command.h
	$(CC) -c server.cpp
client.o: client.cpp Command.h
	$(CC) -c client.cpp

.PHONY: clean
clean:
	rm -f *.o *.out
