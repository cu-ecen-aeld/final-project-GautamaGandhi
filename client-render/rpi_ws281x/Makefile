CROSS_COMPILE=
CC=gcc -Wno-stringop-overflow
SRC=main.c dma.c mailbox.c pcm.c pwm.c rpihw.c ws2811.c seq.c server.c
LIBS=-pthread
CFLAGS=
LINKFLAGS=-lm -lrt

all:
	$(CROSS_COMPILE)$(CC) $(SRC) $(LIBS) $(CFLAGS) $(LINKFLAGS) -o test

clean:
	rm test
