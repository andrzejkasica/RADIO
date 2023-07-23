CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
CFLAGS +=-I.
LDFLAGS = -pthread
#LDFLAGS += -time
binaries = main test
all: main.c # otherfile.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o main main.c
	./main
	make clean
test: test.c # otherfile.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o test test.c
	./test
	make clean
clean:
	rm -f $(binaries) *.o
