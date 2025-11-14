# Cross-platform Makefile for Snake Game
# -------------------------------------------------------------
# Usage (Unix/Linux/Mac):
#   make            # build both normal (snake) and wide (snakew)
#   make snake      # build linked with ncurses
#   make snakew     # build linked with ncursesw (wide chars)
#   make debug      # build normal target with -g
#   make clean      # remove objects and binaries
#
# Windows (MinGW / MSYS2) with PDCurses:
#   Ensure libpdcurses.a (or pdcurses.dll + import lib) is available.
#   Then: make snake  (links against -lpdcurses)
#   Wide build uses same library (PDCurses handles wide internally if built so).
#   If your library name differs, override LIBNCURSES, e.g.:
#       make LIBNCURSES=-lpdcursesw
#
# Customization:
#   make CFLAGS="-O3 -march=native" snake
#   make CURSES_INC="-I/path/to/pdcurses" CURSES_LIBDIR="-L/path/to/pdcurses" snake
# -------------------------------------------------------------

# Detect platform (simple heuristic)
OS := $(shell uname 2>/dev/null || echo Unknown)

# Source files
SRCS := main.c colors.c
OBJS := $(SRCS:.c=.o)

# Default flags (user may override when invoking make)
CFLAGS ?= -Wall -Wextra -O2
CPPFLAGS ?= 
LDFLAGS ?= 

# Optional include/lib directories for curses
CURSES_INC ?= 
CURSES_LIBDIR ?= 

# Library names per platform
ifeq ($(OS),Windows_NT)
    # MinGW / MSYS builds of PDCurses usually ship as -lpdcurses
    LIBNCURSES ?= -lpdcurses
    LIBNCURSESW ?= -lpdcurses
else
    LIBNCURSES ?= -lncurses
    LIBNCURSESW ?= -lncursesw
endif

# Phony targets
.PHONY: all clean debug snake snakew

all: snake snakew

# Normal ncurses build
snake: $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(OBJS) $(CURSES_LIBDIR) $(LIBNCURSES) $(LDFLAGS) -o $@

# Wide-character ncurses build
snakew: $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(OBJS) $(CURSES_LIBDIR) $(LIBNCURSESW) $(LDFLAGS) -o $@

# Debug build (normal ncurses)
debug: CFLAGS += -g
# Ensure objects recompile with debug flags
debug: clean snake

# Pattern rule for object compilation
%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CURSES_INC) -c $< -o $@

clean:
	$(RM) $(OBJS) snake snakew

# Help target for quick reference
.PHONY: help
help:
	@echo "Targets: all snake snakew debug clean help"
	@echo "Override CFLAGS/CPPFLAGS/LDFLAGS, CURSES_INC, CURSES_LIBDIR, LIBNCURSES, LIBNCURSESW as needed"
	@echo "Example: make CURSES_INC='-I../pdcurses' CURSES_LIBDIR='-L../pdcurses' LIBNCURSES='-lpdcurses' snake"
