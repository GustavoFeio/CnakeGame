
CC = cc

FLAGS = -g -Wall -Wextra -pedantic -std=c99

SRC_DIR = ./src

EXE       = cnake_game
DEBUG_EXE = cnake_debug

RAYLIB_DIR = ./thirdparty/raylib

all: $(RAYLIB_DIR)/src/libraylib.a $(EXE) 

debug: $(SRC_DIR)/main.c $(RAYLIB_DIR)/src/libraylib.a
	$(CC) $< -o $(DEBUG_EXE) $(FLAGS) -I$(RAYLIB_DIR)/src -L$(RAYLIB_DIR)/src -lraylib -lm

$(RAYLIB_DIR)/src/libraylib.a: $(RAYLIB_DIR)/src/Makefile
	$(MAKE) -C $(RAYLIB_DIR)/src

$(EXE): $(SRC_DIR)/main.c
	$(CC) $< -o $@ -O3 -I$(RAYLIB_DIR)/src -L$(RAYLIB_DIR)/src -lraylib -lm

clean:
	rm -fv $(EXE) $(DEBUG_EXE)
	$(MAKE) -C $(RAYLIB_DIR)/src clean

