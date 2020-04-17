CC = gcc
CFLAGS = -Wall -g

test: prog2
	./prog2 100 0.1 0.1 10 verbose

prog2: prog2.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -rf prog2
