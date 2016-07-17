CC = gcc
CFLAGS = -std=c99

default:	topgen

all:	topgen readtop complete

topgen:
	$(CC) topgen.c -o topgen $(CFLAGS)

readtop:
	$(CC) readtop.c -o readtop $(CFLAGS)

complete:
	$(CC) complete.c -o complete $(CFLAGS)

clean:
	rm -f readtop
	rm -f topgen
	rm -f complete
