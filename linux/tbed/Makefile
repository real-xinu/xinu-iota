CFLAGS= -c -Wall -std=gnu99
CC=gcc
TARGET=tbed
TARGET2=srbit
TOOLS_P=./ConsoleTools/hdr/
TOOLS_C=./ConsoleTools/lib/*.c
all: $(TARGET)

$(TARGET): $(TARGET).c  $(TARGET2).c
	$(CC)  $(CFLAGS)   $(TARGET).c  $(TARGET2).c  
	$(CC) -o   $(TARGET) $(TARGET).o  $(TARGET2).o -I $(TOOLS_P) $(TOOLS_C)

clean:
	$(RM) $(TARGET) $(TARGET).o $(TARGET2).o $(TARGET).c.orig $(TARGET2).c.orig  *.o 
