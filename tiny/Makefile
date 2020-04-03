CC = gcc
CFLAGS = -Og -Wuninitialized -Wall -I .


all: tiny cgi

tiny: tiny.c csapp.o csapp.h
	$(CC) $(CFLAGS) -o tiny tiny.c csapp.c 

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c
cgi:
	(cd cgi-bin; make)

clean:
	rm -f *.o tiny *~
	(cd cgi-bin; make clean)

