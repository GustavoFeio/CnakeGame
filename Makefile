CC = clang

SRC    = ./src
RAYLIB = ./thirdparty/raylib
CLOG   = ./thirdparty/clog

FLAGS = -g -Wall -Wextra -DCNAKE_DEBUG
INCLUDES = -I$(RAYLIB)/src -I$(CLOG)/lib
LINKS = -L$(RAYLIB)/src -L$(CLOG)/lib
LIBS = -lraylib -lm

ifeq ($(shell uname -s),Darwin)
	LIBS += -framework Cocoa -framework IOKit -framework OpenGL -framework CoreVideo -framework QuartzCore
endif

EXE       = cnake_game
DEBUG_EXE = cnake_debug

.PHONY: all clean
all:   $(EXE)
debug: $(DEBUG_EXE)

$(EXE): $(SRC)/main.c $(RAYLIB)/src/libraylib.a
	$(CC) $< -o $@ -O3 $(INCLUDES) $(LINKS) $(LIBS)

$(DEBUG_EXE): $(SRC)/main.c $(RAYLIB)/src/libraylib.a
	$(CC) $< -o $@ $(FLAGS) $(INCLUDES) $(LINKS) $(LIBS)

$(RAYLIB)/src/libraylib.a: $(RAYLIB)/src/Makefile
	$(MAKE) -C $(RAYLIB)/src

clean:
	rm -frv $(EXE) $(DEBUG_EXE)*
	$(MAKE) -C $(RAYLIB)/src clean
