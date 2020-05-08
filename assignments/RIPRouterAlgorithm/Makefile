CC = gcc
CFLAGS = -Wall -g

all: prog3

prog3: ./obj/*.o
	$(CC) $(CFLAGS) -o $@ $^

./obj/%.o : ./src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: prog3
	./prog3

clean:
	rm -rf prog3 ./obj/*
