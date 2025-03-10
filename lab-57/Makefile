CC=gcc
CFLAGS=-Wall -Wextra -std=c99

all: lab

lab: main.o node.o
	$(CC) $(CFLAGS) -o lab main.o node.o

main.o: main.c node.h
	$(CC) $(CFLAGS) -c main.c

node.o: node.c node.h
	$(CC) $(CFLAGS) -c node.c

clean:
	rm -f lab *.o
