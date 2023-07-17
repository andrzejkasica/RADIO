CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
CFLAGS +=-I.
LDFLAGS = -pthread
#LDFLAGS += -time
binaries = main
all: main.c # otherfile.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o main main.c
	./main

clean:
	rm -f $(binaries) *.o
