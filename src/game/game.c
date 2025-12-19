// src/game/game.c
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "select_musics.h"
#include "beatmap.h"
#include "ingame.h"
#include "audio.h"

extern char g_nick[];

int game_main(void)
{
    const char *song_path = select_music();
    if (!song_path || !*song_path) {
        fprintf(stderr, "No music selected\n");
        return 1;
    }

    char beatmap_path[512];
    generate_beatmap_path(song_path, beatmap_path, sizeof(beatmap_path));

    BeatMap *bm = beatmap_load(beatmap_path);
    if (!bm) {
        fprintf(stderr, "Failed to load beatmap: %s\n", beatmap_path);
        return 1;
    }

    if (!audio_init()) {
        beatmap_free(bm);
        return 1;
    }

    gpio_t gpio;
    if (!gpio_init(&gpio, GPIO_LEFT, GPIO_RIGHT, GPIO_HIT)) {
        audio_close();
        beatmap_free(bm);
        return 1;
    }

    if (!audio_play_bgm(song_path)) {
        gpio_close(&gpio);
        audio_close();
        beatmap_free(bm);
        return 1;
    }

    GameResult gr = play_game_ncurses(bm, &gpio, g_nick);
    (void)gr;

    gpio_close(&gpio);
    audio_stop();
    audio_close();
    beatmap_free(bm);
    return 0;
}



