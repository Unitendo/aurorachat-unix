CC=gcc
CFLAGS=-Wall -Iinclude --std=c99

LD=gcc
LFLAGS=

INC=$(wildcard include/*.h)
SRC=$(wildcard src/*.c)
OBJ=$(patsubst src/%.c,bin/%.o,$(SRC))
BIN=bin/aurorachat
RUN=$(BIN)

.PHONY: all

all: $(BIN)

clean:
	rm bin/*

run: all
	$(RUN)

bin/%.o: src/%.c $(INC)
	$(CC) $(CFLAGS) -o $@ -c $(filter-out %.h,$^)

$(BIN): $(OBJ)
	$(LD) $(LFLAGS) -o $@ $^
