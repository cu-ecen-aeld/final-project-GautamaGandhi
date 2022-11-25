INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc -Wno-stringop-overflow

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

all:	server

clean:
	-rm -f *.o *.d
	-rm -f server

server: server.o
	$(CC) $(LDFLAGS) $(CFLAGS) seq.c server.c -o server -lm -lpthread

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<