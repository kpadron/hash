# Makefile
# kpadron.github@gmail.com
# Kristian Padron
CC := gcc

CFLAGS := -Wall -Wextra
DEBUG := -g -Og

INC := -I inc
VPATH := src

RM := -rm -f *.o *~ core

BINS := hash-test hash-table-test

all: $(BINS)

hash-test: hash-test.c hash.o
	$(CC) $(CFLAGS) $(DEBUG) -o $@ $^ $(INC)

hash-table-test: hash-table-test.c hash.o hash-table.o
	$(CC) $(CFLAGS) $(DEBUG) -o $@ $^ $(INC)

%.o: %.c
	$(CC) $(CFLAGS) $(DEBUG) -c -o $@ $< $(INC)

%: %.c
	$(CC) $(CFLAGS) $(DEBUG) -o $@ $<

r: clean all

clean:
	$(RM) $(BINS)
