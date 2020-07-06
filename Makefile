CC=gcc
CFLAGS=-Wall -Wextra -pedantic -g -std=gnu99 -lm
TARGETS=2310hub 2310alice 2310bob

.DEFAULT: all

all: $(TARGETS)

utilities.o: utilities.c utilities.h
		$(CC) $(CFLAGS) -c utilities.c -o utilities.o

players.o: players.c players.h
		$(CC) $(CFLAGS) -c players.c -o players.o

2310hub: hub.c utilities.o
		$(CC) $(CFLAGS) utilities.o hub.c -o 2310hub

2310alice: alice.c players.o utilities.o
		$(CC) $(CFLAGS) utilities.o players.o alice.c -o 2310alice

2310bob: bob.c players.o utilities.o
		$(CC) $(CFLAGS) utilities.o players.o bob.c -o 2310bob

clean:
		rm -f $(TARGETS) *.o