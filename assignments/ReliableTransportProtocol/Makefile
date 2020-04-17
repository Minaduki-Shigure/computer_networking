CC = gcc
CFLAGS = -Wall -g

all: prog2 prog2plus

prog2: prog2.c
	$(CC) $(CFLAGS) -o $@ $^

prog2plus: prog2.c
	$(CC) $(CFLAGS) -D BIDIRECTIONAL -o $@ $^

test: prog2
	./prog2 100 0.1 0.3 1000 2

testplus: prog2plus
	./prog2plus 100 0.1 0.3 1000 2

clean:
	rm -rf prog2
