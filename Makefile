CC = gcc
CFLAGS = -Wall -pthread
TARGET = copy_tool

all:
	$(CC) $(CFLAGS) main.c queue.c scanner.c worker.c -o $(TARGET)

clean:
	rm -f $(TARGET)
