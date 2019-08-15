# Makefile
# kpadron.github@gmail.com
# Kristian Padron
CC := gcc

CFLAGS := -Wall
DEBUG := -g -Og
OPT := -Ofast
MODE := $(OPT)

INC := -I inc
VPATH := src

RM := -rm -f *.o *~ core

BINS := hash-test hash-table-test

all: $(BINS)

hash-test: hash-test.c hash.o
	$(CC) $(CFLAGS) $(MODE) -o $@ $^ $(INC)

hash-table-test: hash-table-test.c hash.o hash-table.o
	$(CC) $(CFLAGS) $(MODE) -o $@ $^ $(INC)

%.o: %.c
	$(CC) $(CFLAGS) $(MODE) -c -o $@ $< $(INC)

%: %.c
	$(CC) $(CFLAGS) $(MODE) -o $@ $<

r: clean all

clean:
	$(RM) $(BINS)
