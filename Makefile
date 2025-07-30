
CC=cc

FLAGS=-g -Wall -Wextra -pedantic -std=c99

SRC_DIR=./src

EXE=cnake_game

RAYLIB_DIR=./thirdparty/raylib

all: $(RAYLIB_DIR)/src/libraylib.a $(EXE) 

$(RAYLIB_DIR)/src/libraylib.a: $(RAYLIB_DIR)/src/Makefile
	$(MAKE) -C $(RAYLIB_DIR)/src

$(EXE): $(SRC_DIR)/main.c
	$(CC) $< -o $@ $(FLAGS) -I$(RAYLIB_DIR)/src -L$(RAYLIB_DIR)/src -lraylib -lm

clean:
	rm -fv $(EXE)
	$(MAKE) -C $(RAYLIB_DIR)/src clean

