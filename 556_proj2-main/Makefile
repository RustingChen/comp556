
CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g

LDFLAGS	 	= 
DEFS 	 	=

all:	recvfile sendfile

recvfile: recvfile.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o recvfile recvfile.c

sendfile: sendfile.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o sendfile sendfile.c

clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f recvfile
	rm -f sendfile