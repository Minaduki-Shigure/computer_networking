CC = gcc
CFLAGS = -Wall -g

vpath %.o ./obj/

prog: ./src/prog3.c ./src/node.c ./src/node1.c ./src/node2.c ./src/node3.c 
	$(CC) $(CFLAGS) -o $@ $^

all: prog3

prog3: ./obj/prog3.o ./obj/node.o ./obj/node1.o ./obj/node2.o ./obj/node3.o 
	$(CC) $(CFLAGS) -o $@ $^

./obj/%.o : ./src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: prog3
	./prog3

clean:
	rm -rf prog3 ./obj/*
