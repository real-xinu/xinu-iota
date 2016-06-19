CFLAGS = -c -g -Wall -std=gnu99
CC = gcc
TARGET = tbed
TARGET2 = srbit
all: $(TARGET)

$(TARGET): $(TARGET).c  $(TARGET2).c
	$(CC) $(CFLAGS)    $(TARGET).c  $(TARGET2).c
	$(CC) -o $(TARGET) $(TARGET).o  $(TARGET2).o

clean:
	$(RM) $(TARGET) $(TARGET).o $(TARGET2).o 
