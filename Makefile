# bbg_tui — Bloomberg POMS Position Manager
# Built on a fork of rxi/microui
#
# Prerequisites:
#   - SDL2 dev libs (brew install sdl2 / apt install libsdl2-dev)
#   - OpenGL
#   - Copy from rxi/microui repo into lib/:
#       microui.h  (or use our modified version)
#       microui.c
#     And into demo/:
#       renderer.h
#       renderer.c
#       atlas.inl

CC       = gcc
CFLAGS   = -O2 -Wall -Wextra -Wpedantic -std=c11
CFLAGS  += -I./lib -I./src -I./demo
CFLAGS  += $(shell sdl2-config --cflags)

LDFLAGS  = $(shell sdl2-config --libs) -lm

UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
  LDFLAGS += -framework OpenGL
else
  LDFLAGS += -lGL
endif

# ---- Sources ----

LIB_SRC  = lib/microui.c
SRC_SRC  = src/data.c src/table.c src/screen.c src/poms.c
DEMO_SRC = demo/main.c demo/renderer.c

ALL_SRC  = $(LIB_SRC) $(SRC_SRC) $(DEMO_SRC)
ALL_OBJ  = $(ALL_SRC:.c=.o)

BIN      = poms

# ---- Build Rules ----

all: check_deps $(BIN)

$(BIN): $(ALL_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Header Dependencies ----

lib/microui.o:    lib/microui.c lib/microui.h
demo/renderer.o:  demo/renderer.c demo/renderer.h lib/microui.h demo/atlas.inl
demo/main.o:      demo/main.c lib/bbg_tui.h lib/microui.h demo/renderer.h \
                  src/theme.h src/data.h src/screen.h src/poms.h
src/data.o:       src/data.c src/data.h
src/table.o:      src/table.c src/table.h src/theme.h lib/bbg_tui.h
src/screen.o:     src/screen.c src/screen.h src/theme.h lib/bbg_tui.h src/data.h
src/poms.o:       src/poms.c src/poms.h src/table.h src/theme.h src/screen.h \
                  lib/bbg_tui.h src/data.h

# ---- Check for required files ----

check_deps:
	@test -f lib/microui.c || \
	  (echo "ERROR: lib/microui.c not found. Copy from rxi/microui repo." && exit 1)
	@test -f demo/renderer.c || \
	  (echo "ERROR: demo/renderer.c not found. Copy from rxi/microui demo/." && exit 1)
	@test -f demo/atlas.inl || \
	  (echo "ERROR: demo/atlas.inl not found. Copy from rxi/microui demo/." && exit 1)

# ---- Utility ----

clean:
	rm -f $(ALL_OBJ) $(BIN)

run: all
	./$(BIN)

.PHONY: all clean run check_deps
