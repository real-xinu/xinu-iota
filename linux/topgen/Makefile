CC = gcc
CFLAGS = -std=c99

default:	topgen

all:	topgen readtop

topgen:
	$(CC) topgen.c -o topgen $(CFLAGS)

readtop:
	$(CC) readtop.c -o readtop $(CFLAGS)

clean:
	rm -f readtop
	rm -f topgen
