CC = gcc
CFLAGS = -Wall -Wextra
target = server
src = src/server.c 

all:
	$(CC) $(CFLAGS) $(src) -o $(target) 

clean:
	rm $(target)
