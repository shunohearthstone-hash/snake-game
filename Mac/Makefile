# Executable name
TARGET = snake

# Source files
SRC = main.c colors.c

# Compiler
CC = clang

# Compiler flags
CFLAGS = -Wall -Wextra -O2

# Try to locate Homebrew ncurses
BREW_PREFIX := $(shell brew --prefix ncurses 2>/dev/null)

ifeq ($(BREW_PREFIX),)
    # Use macOS system ncurses
    NCURSES_LIBS = -lncurses
else
    # Use Homebrew ncurses
    CFLAGS += -I$(BREW_PREFIX)/include
    LDFLAGS += -L$(BREW_PREFIX)/lib
    NCURSES_LIBS = -lncurses
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(TARGET) $(NCURSES_LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
