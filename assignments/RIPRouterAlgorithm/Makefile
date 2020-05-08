CC = gcc
CFLAGS = -Wall -g

vpath %.o ./obj/

all: prog3

prog3: *.o
	$(CC) $(CFLAGS) -o $@ $^

./obj/%.o : ./src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: prog3
	./prog3

clean:
	rm -rf prog3 ./obj/*
