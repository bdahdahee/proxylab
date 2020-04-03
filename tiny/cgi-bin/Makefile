CC = gcc
CFLAGS = -Og -g -Wall -Wuninitialized -I..

all: adder

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c

clean:
	rm -f adder *~
