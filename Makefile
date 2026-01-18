
CC = cc

SRC    = ./src
RAYLIB = ./thirdparty/raylib
CLOG   = ./thirdparty/clog

FLAGS = -g -Wall -Wextra
INCLUDES = -I$(RAYLIB)/src -I$(CLOG)/lib
LINKS = -L$(RAYLIB)/src -L$(CLOG)/lib
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

