CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2

all: dag_runner

dag_runner: main.o
	$(CC) $(CFLAGS) -o dag_runner main.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f dag_runner main.o
