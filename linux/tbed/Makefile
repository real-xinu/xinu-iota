CFLAGS= -c -Wall -std=gnu99
CC=gcc
ALL= ./src/tbed.c ./src/srbit.c ./src/help.c ./src/isnum.c  ./src/cmdhandler.c \
     ./src/map.c  ./src/topdump.c ./src/ts_find.c ./src/csconsole.c ./src/ping.c \
     ./src/resp_handler.c ./src/error_handler.c ./src/udp.c ./src/result.c
OBJ= ./src/tbed.o ./src/srbit.o ./src/help.o ./src/isnum.o ./src/cmdhandler.o ./src/map.o \
     ./src/topdump.o ./src/ts_find.o ./src/csconsole.o ./src/ping.o ./src/resp_handler.o \
     ./src/error_handler.o ./src/udp.o ./src/result.o

SOURCE=./src/
TARGET=./src/tbed
TOOLS_P=./ConsoleTools/hdr/
TOOLS_C=./ConsoleTools/lib/*.c
HEADER=./include/headers.h
MACROS=./include/macros.h
GLOBAL=./include/global.h


all: $(OBJ)   $(HEADER) $(MACROS) 
	$(CC) -o   $(TARGET) $(OBJ) -I $(TOOLS_P) $(TOOLS_C)

$(TARGET): $(ALL) $(HEADER) $(MACROS) $(GLOBAL)
	$(CC) $(CFLAGS) $(ALL)

$(TARGET9): $(TARGET9).c $(HEADER) $(MACROS) $(GLOBAL) 
	$(CC) $(CFLAGS) $(TARGET9).c -I $(TOOLS_P) $(TOOLS_C)


clean:
	$(RM) $(TARGET)  $(SOURCE)*.o  $(SOURCE)*.c.orig log.txt 
