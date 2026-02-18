# bbg_tui — Bloomberg POMS Position Manager
# Built on a fork of rxi/microui
#
# Usage:  make help

# ---- Toolchain ----
CC       := gcc
CSTD     := -std=c11

# ---- Strict Warnings (NASA Power of Ten spirit) ----
WARNINGS := -Wall -Wextra -Wpedantic            \
            -Werror                               \
            -Wshadow                              \
            -Wdouble-promotion                    \
            -Wformat=2                            \
            -Wformat-overflow=2                   \
            -Wformat-truncation=2                 \
            -Wnull-dereference                    \
            -Wuninitialized                       \
            -Wstrict-prototypes                   \
            -Wold-style-definition                \
            -Wmissing-prototypes                  \
            -Wmissing-declarations                \
            -Wredundant-decls                     \
            -Wcast-align                          \
            -Wundef                               \
            -Wstack-usage=4096                    \
            -Wno-unused-parameter

# Conversion warnings: enabled but non-fatal for now.
# microui's API uses int everywhere (sizes, lengths, coords)
# which creates friction with size_t from strlen etc.
# Goal: promote to -Werror once all sites use explicit casts.
WARNINGS += -Wconversion -Wsign-conversion       \
            -Wno-error=conversion                 \
            -Wno-error=sign-conversion            \
            -Wno-error=double-promotion

# ---- SDL2 Detection ----
# Prefer sdl2-config, fall back to pkg-config, fall back to manual
SDL2_CFLAGS  := $(shell sdl2-config --cflags 2>/dev/null || pkg-config --cflags sdl2 2>/dev/null || echo "-I/usr/include/SDL2")
SDL2_LDFLAGS := $(shell sdl2-config --libs   2>/dev/null || pkg-config --libs   sdl2 2>/dev/null || echo "-lSDL2")

# ---- Include Paths ----
INCLUDES := -I./lib -I./src -I./demo

# ---- Compiler Flags ----
CFLAGS_COMMON := $(CSTD) $(WARNINGS) $(INCLUDES) $(SDL2_CFLAGS)
CFLAGS_DEBUG  := $(CFLAGS_COMMON) -O0 -g3 -DDEBUG -fsanitize=address,undefined
CFLAGS_RELEASE:= $(CFLAGS_COMMON) -O2 -DNDEBUG -march=native -flto

# Default build is debug
CFLAGS := $(CFLAGS_DEBUG)

# ---- Linker ----
UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
  GL_LDFLAGS := -framework OpenGL
else
  GL_LDFLAGS := -lGL
endif

LDFLAGS_COMMON := $(SDL2_LDFLAGS) $(GL_LDFLAGS) -lm
LDFLAGS_DEBUG  := $(LDFLAGS_COMMON) -fsanitize=address,undefined
LDFLAGS_RELEASE:= $(LDFLAGS_COMMON) -flto

LDFLAGS := $(LDFLAGS_DEBUG)

# ---- Sources & Objects ----
LIB_SRC  := lib/microui.c
SRC_SRC  := src/data.c src/table.c src/screen.c src/poms.c
DEMO_SRC := demo/main.c demo/renderer.c

ALL_SRC  := $(LIB_SRC) $(SRC_SRC) $(DEMO_SRC)
ALL_OBJ  := $(ALL_SRC:.c=.o)

BIN      := poms

# ---- Default Target ----
.DEFAULT_GOAL := build

# ============================================================================
#  Targets
# ============================================================================

.PHONY: help build release run run-release valgrind clean check_deps

help: ## Show this help
	@echo ""
	@echo "  bbg_tui — Bloomberg POMS Position Manager"
	@echo "  =========================================="
	@echo ""
	@echo "  Targets:"
	@echo "    make build        Debug build   (-O0, -g3, ASan+UBSan)"
	@echo "    make release      Release build (-O2, -march=native, -flto)"
	@echo "    make run          Build debug and run"
	@echo "    make run-release  Build release and run"
	@echo "    make valgrind     Build without ASan and run under valgrind"
	@echo "    make clean        Remove all build artifacts"
	@echo "    make help         Show this message"
	@echo ""
	@echo "  Keyboard shortcuts:"
	@echo "    F1-F4             Switch to screen 1-4"
	@echo "    Ctrl+T            Add new screen"
	@echo "    Double-click tab  Rename screen"
	@echo "    Right-click tab   Close screen"
	@echo ""
	@echo "  Prerequisites:"
	@echo "    sudo apt install libsdl2-dev valgrind  (Debian/Ubuntu)"
	@echo "    brew install sdl2                      (macOS)"
	@echo ""

build: CFLAGS  = $(CFLAGS_DEBUG)
build: LDFLAGS = $(LDFLAGS_DEBUG)
build: check_deps $(BIN) ## Debug build (default)
	@echo "[OK] Debug build complete: ./$(BIN)"

release: CFLAGS  = $(CFLAGS_RELEASE)
release: LDFLAGS = $(LDFLAGS_RELEASE)
release: check_deps $(BIN) ## Release build
	@echo "[OK] Release build complete: ./$(BIN)"

run: build ## Build debug and run
	@echo "[RUN] Starting $(BIN)..."
	@LSAN_OPTIONS=suppressions=bbg_tui_lsan.supp ./$(BIN)

run-release: release ## Build release and run
	@echo "[RUN] Starting $(BIN) (release)..."
	@./$(BIN)

# Valgrind and ASan conflict, so build without sanitizers
CFLAGS_VALGRIND  := $(CFLAGS_COMMON) -O0 -g3 -DDEBUG
LDFLAGS_VALGRIND := $(LDFLAGS_COMMON)

valgrind: CFLAGS  = $(CFLAGS_VALGRIND)
valgrind: LDFLAGS = $(LDFLAGS_VALGRIND)
valgrind: clean check_deps $(BIN) ## Build without ASan and run under valgrind
	@echo "[VALGRIND] Starting $(BIN)..."
	valgrind --leak-check=full          \
	         --show-leak-kinds=all      \
	         --track-origins=yes        \
	         --suppressions=bbg_tui.supp \
	         --errors-for-leak-kinds=definite \
	         --error-exitcode=1         \
	         ./$(BIN)

# ---- Link ----
$(BIN): $(ALL_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# ---- Compile: our code gets strict flags ----
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Compile: upstream code gets relaxed flags (not our warnings to fix) ----
CFLAGS_UPSTREAM = $(CSTD) -O2 -Wall -Wextra $(INCLUDES) $(SDL2_CFLAGS) -Wno-unused-parameter

lib/microui.o: lib/microui.c lib/microui.h
	$(CC) $(CFLAGS_UPSTREAM) -c $< -o $@

demo/renderer.o: demo/renderer.c demo/renderer.h lib/microui.h demo/atlas.inl
	$(CC) $(CFLAGS_UPSTREAM) -c $< -o $@

# ---- Header Dependencies ----
demo/main.o:      demo/main.c lib/bbg_tui.h lib/microui.h demo/renderer.h \
                  src/theme.h src/data.h src/screen.h src/poms.h
src/data.o:       src/data.c src/data.h
src/table.o:      src/table.c src/table.h src/theme.h lib/bbg_tui.h
src/screen.o:     src/screen.c src/screen.h src/theme.h lib/bbg_tui.h src/data.h
src/poms.o:       src/poms.c src/poms.h src/table.h src/theme.h src/screen.h \
                  lib/bbg_tui.h src/data.h

# ---- Dependency Check ----
check_deps:
	@test -f lib/microui.c  || (echo "ERROR: lib/microui.c missing.  Copy from rxi/microui src/"  && exit 1)
	@test -f demo/renderer.c || (echo "ERROR: demo/renderer.c missing. Copy from rxi/microui demo/" && exit 1)
	@test -f demo/renderer.h || (echo "ERROR: demo/renderer.h missing. Copy from rxi/microui demo/" && exit 1)
	@test -f demo/atlas.inl  || (echo "ERROR: demo/atlas.inl missing.  Copy from rxi/microui demo/" && exit 1)

# ---- Clean ----
clean: ## Remove all build artifacts
	rm -f $(ALL_OBJ) $(BIN)
	@echo "[OK] Cleaned."
