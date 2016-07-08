CC=gcc
TOOLS_P=./ConsoleTools/hdr/
TOOLS_C=./ConsoleTools/lib/*.c

all:	console

# Command for console executable
console: console_connect.c
	$(CC) -o console console_connect.c -I $(TOOLS_P) $(TOOLS_C)

clean:
	rm -f console
