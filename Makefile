CC = gcc
CFLAGS = -Wall -Wextra
target = bin/server
src = src/server.c 

all:
	mkdir -p bin/
	$(CC) $(CFLAGS) $(src) -o $(target) 

clean:
	rm -rf bin/
