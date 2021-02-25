CC=gcc
CFLAGS= -Wall -Wextra -Wpedantic -Werror -pthread

all: test 

pool.o: pool.c pool.h
test: test.c pool.o

.PHONY:
clean:
	rm -f test *.o
