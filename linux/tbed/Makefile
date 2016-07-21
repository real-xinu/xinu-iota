CFLAGS= -c -Wall -std=gnu99
CC=gcc
TARGET=./src/tbed
TARGET2=./src/srbit
TARGET3=./src/help
TARGET4=./src/isnum
TARGET5=./src/cmdhandler
TOOLS_P=./ConsoleTools/hdr/
TOOLS_C=./ConsoleTools/lib/*.c
HEADER=./include/headers.h
MACROS=./include/macros.h
GLOBAL=./include/global.h
all: $(TARGET).o $(TARGET2).o $(TARGET3).o  $(TARGET4).o $(TARGET5).o $(HEADER) $(MACROS)
	$(CC) -o   $(TARGET) $(TARGET).o  $(TARGET2).o $(TARGET3).o  $(TARGET4).o  $(TARGET5).o  -I $(TOOLS_P) $(TOOLS_C)

$(TARGET): $(TARGET).c  $(TARGET2).c $(TARGET3).c $(HEADER) $(MACROS)
	$(CC)  $(CFLAGS)   $(TARGET).c  $(TARGET2).c

$(TARGET3): $(TAEGET3).c $(HEADER) $(MACROS) $(GLOBAL)
	$(CC) $(CFLAGS) $(TARGET3).c

$(TARGET4): $(TARGET4).c $(HEADER) $(MACROS) $(GLOBAL)
	$(CC) $(CFLAGS) $(TARGET4).c


$(TARGET5): $(TARGET5).c $(HEADER) $(MACROS) $(GLOBAL)
	$(CC) $(CFLAGS) $(TARGET5).c




clean:
	$(RM) $(TARGET) $(TARGET).o $(TARGET2).o $(TARGET3).o $(TARGET4).o  $(TARGET).c.orig $(TARGET2).c.orig $(TARGET3).c.orig  *.o log.txt 
