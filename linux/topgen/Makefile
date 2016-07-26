CC = gcc
CFLAGS = -std=c99

default:	topgen createtop

all:	topgen readtop complete star linear createtop

topgen:
	$(CC) topgen.c -o topgen $(CFLAGS)

readtop:
	$(CC) readtop.c -o readtop $(CFLAGS)

complete:
	$(CC) complete.c -o complete $(CFLAGS)

star:
	$(CC) star.c -o star $(CFLAGS)

linear:
	$(CC) linear.c -o linear $(CFLAGS)

createtop:
	$(CC) createtop.c -o createtop $(CFLAGS)

clean:
	rm -f readtop
	rm -f topgen
	rm -f complete
	rm -f star
	rm -f linear
	rm -f createtop
