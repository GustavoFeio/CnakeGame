
CC=cc

FLAGS=`sdl2-config --cflags --libs` -g -lm -Wall -Wextra -pedantic -std=c2x

SRC=./src/
BIN=./bin/

EXE=snake_game

all: $(EXE)

$(EXE): $(SRC)main.c
	$(CC) $< -o $@ $(FLAGS)

bin:
	mkdir $(BIN)

clean:
	rm -r $(BIN)

