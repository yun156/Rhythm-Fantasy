#pragma once
#include "common.h"
#include "beatmap.h"
#include "gpio_input.h"

typedef struct {
    int score, great, good, miss, combo, max_combo;
} GameResult;

GameResult play_game_ncurses(BeatMap *bm, gpio_t *gpio, const char *nick);



