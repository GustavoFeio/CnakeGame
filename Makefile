
CC=cc

FLAGS=-g -Wall -Wextra -pedantic -std=c99

SRC=./src/
BIN=./bin/

EXE=cnake_game

all: $(EXE)

$(EXE): $(SRC)main.c
	$(CC) $< -o $@ $(FLAGS) -I/usr/local/include/ -L/usr/local/lib/ -lraylib -lm

bin:
	mkdir $(BIN)

clean:
	rm -r $(BIN)

