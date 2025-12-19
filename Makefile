# ========= Project layout =========
SRCDIR   := src/game
OBJDIR   := build
BINDIR   := bin

# 모든 .c 수집 (server.c는 game에서 제외)
SRC_ALL      := $(wildcard $(SRCDIR)/*.c)
SRC_GAME     := $(filter-out $(SRCDIR)/server.c $(SRCDIR)/rank.c,$(SRC_ALL))

SRC_SERVER   := $(SRCDIR)/server.c

OBJS_GAME    := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRC_GAME))
OBJS_SERVER  := $(OBJDIR)/server.o

# ========= Toolchain & flags =========
CC      ?= gcc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra
INCS    := -Isrc/game

CFLAGS += -include src/game/game_compat.h
# 이미 있는 규칙들 아래쪽에 추가 (경로는 네 Makefile 기준)
$(OBJDIR)/audio.o: CFLAGS += -DBUILDING_AUDIO


# ncurses(w), pthread, libgpiod 링크
# (시스템에 따라 ncursesw가 필요할 수 있어서 우선 탐색)
CURSES_LIBS := $(shell pkg-config --libs ncursesw 2>/dev/null || pkg-config --libs ncurses 2>/dev/null || echo "-lncurses")
CURSES_CFLAGS := $(shell pkg-config --cflags ncursesw 2>/dev/null || pkg-config --cflags ncurses 2>/dev/null)

LIBS    := $(CURSES_LIBS) -lpthread -lgpiod
CFLAGS  += $(CURSES_CFLAGS)

# SDL_mixer를 쓰고 싶으면: make SDL=1
ifeq ($(SDL),1)
  CFLAGS += -DUSE_SDL
  LIBS   += -lSDL2 -lSDL2_mixer
endif

# ========= Targets =========
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



