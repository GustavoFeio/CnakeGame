
CC = cc

SRC = ./src
RAYLIB = ./thirdparty/raylib

FLAGS = -g -Wall -Wextra -pedantic -std=c99
INCLUDES = -I$(RAYLIB)/src
LINKS = -L$(RAYLIB)/src
LIBS = -l:libraylib.a -lm

EXE       = cnake_game
DEBUG_EXE = cnake_debug

all: $(EXE)

$(EXE): $(SRC)/main.c $(RAYLIB)/src/libraylib.a
	$(CC) $< -o $@ -O3 $(INCLUDES) $(LINKS) $(LIBS)

debug: $(SRC)/main.c $(RAYLIB)/src/libraylib.a
	$(CC) $< -o $(DEBUG_EXE) $(FLAGS) $(INCLUDES) $(LINKS) $(LIBS)

$(RAYLIB)/src/libraylib.a: $(RAYLIB)/src/Makefile
	$(MAKE) -C $(RAYLIB)/src

.PHONY: clean
clean:
	rm -fv $(EXE) $(DEBUG_EXE)
	$(MAKE) -C $(RAYLIB)/src clean

