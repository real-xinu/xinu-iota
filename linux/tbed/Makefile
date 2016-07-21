CFLAGS= -c -Wall -std=gnu99
CC=gcc
SOURCE=./src/
TARGET=./src/tbed
TARGET2=./src/srbit
TARGET3=./src/help
TARGET4=./src/isnum
TARGET5=./src/cmdhandler
TARGET6=./src/map
TARGET7=./src/topdump
TARGET8=./src/ts_find
TARGET9=./src/csconsole
TARGET10=./src/ping
TARGET11=./src/resp_handler
TARGET12=./src/error_handler
TARGET13=./src/udp
TOOLS_P=./ConsoleTools/hdr/
TOOLS_C=./ConsoleTools/lib/*.c
HEADER=./include/headers.h
MACROS=./include/macros.h
GLOBAL=./include/global.h
all: $(TARGET).o $(TARGET2).o $(TARGET3).o  $(TARGET4).o $(TARGET5).o $(TARGET6).o $(TARGET7).o \
	$(TARGET8).o  $(TARGET9).o  $(TARGET10).o  $(TARGET11).o  $(TARGET12).o $(TARGET13).o  $(HEADER) $(MACROS) 
	$(CC) -o   $(TARGET) $(TARGET).o  $(TARGET2).o $(TARGET3).o  $(TARGET4).o  $(TARGET5).o $(TARGET6).o \
	$(TARGET7).o $(TARGET8).o  $(TARGET9).o $(TARGET10).o $(TARGET11).o $(TARGET12).o $(TARGET13).o -I $(TOOLS_P) $(TOOLS_C)

$(TARGET): $(TARGET).c $(TARGET2).c $(TAEGET3).c $(TARGET4).c $(TARGET5).c $(TARGET6).c \
	$(TARGET7).c $(TARGET8).c  $(TARGET12).c  $(TARGET13).c $(HEADER) $(MACROS) $(GLOBAL)
	$(CC) $(CFLAGS) $(TARGET).c $(TARGET2).c $(TARGET3).c $(TARGET4).c $(TARGET5).c \
	$(TARGET6).c $(TARGET7).c $(TARGET8).c $(TARGET10).c $(TARGET11).c $(TARGET12).c $(TARGET13).c

$(TARGET9): $(TARGET9).c $(HEADER) $(MACROS) $(GLOBAL) 
	$(CC) $(CFLAGS) $(TARGET9).c -I $(TOOLS_P) $(TOOLS_C)


clean:
	$(RM) $(TARGET)  $(SOURCE)*.o  $(SOURCE)*.c.orig log.txt 
