
SRCDIR   := src/game
OBJDIR   := build
BINDIR   := bin

SRC_ALL      := $(wildcard $(SRCDIR)/*.c)
SRC_GAME     := $(filter-out $(SRCDIR)/server.c $(SRCDIR)/rank.c,$(SRC_ALL))

SRC_SERVER   := $(SRCDIR)/server.c

OBJS_GAME    := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRC_GAME))
OBJS_SERVER  := $(OBJDIR)/server.o

CC      ?= gcc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra
INCS    := -Isrc/game

CFLAGS += -include src/game/game_compat.h

$(OBJDIR)/audio.o: CFLAGS += -DBUILDING_AUDIO


CURSES_LIBS := $(shell pkg-config --libs ncursesw 2>/dev/null || pkg-config --libs ncurses 2>/dev/null || echo "-lncurses")
CURSES_CFLAGS := $(shell pkg-config --cflags ncursesw 2>/dev/null || pkg-config --cflags ncurses 2>/dev/null)

LIBS    := $(CURSES_LIBS) -lpthread -lgpiod
CFLAGS  += $(CURSES_CFLAGS)

ifeq ($(SDL),1)
  CFLAGS += -DUSE_SDL
  LIBS   += -lSDL2 -lSDL2_mixer
endif

.PHONY: all dirs clean run run-server

all: dirs game server

dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

game: $(OBJS_GAME)
	$(CC) $(CFLAGS) -o $(BINDIR)/$@ $^ $(LIBS)

server: $(OBJS_SERVER)
	$(CC) $(CFLAGS) -o $(BINDIR)/$@ $^ $(LIBS)

run: all
	$(BINDIR)/game

run-server: server
	$(BINDIR)/server

clean:
	rm -rf $(OBJDIR) $(BINDIR)



