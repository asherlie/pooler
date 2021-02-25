CC=gcc
CFLAGS= -Wall -Wextra -Wpedantic -Werror -pthread

all: test 

pool.o: pool.c pool.h
test: test.c pool.o
db: test.c pool.c
	$(CC) test.c pool.c -DDEBUG -pthread -o db
	

.PHONY:
clean:
	rm -f db test *.o
