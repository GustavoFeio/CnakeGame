
CC=cc

FLAGS=`sdl2-config --cflags --libs` -g -lm -Wall -Wextra -pedantic -std=c99

SRC=./src/
BIN=./bin/

EXE=cnake_game

all: $(EXE)

$(EXE): $(SRC)main.c
	$(CC) $< -o $@ $(FLAGS)

bin:
	mkdir $(BIN)

clean:
	rm -r $(BIN)

