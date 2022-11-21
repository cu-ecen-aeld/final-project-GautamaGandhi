CROSS_COMPILE=
CC=gcc
SRC=main.c
LIBS=-lpigpio -pthread
CFLAGS=

all:
	$(CROSS_COMPILE)$(CC) $(SRC) $(LIBS) $(CFLAGS) -o pigpio-test

clean:
	rm $(PROJ)
