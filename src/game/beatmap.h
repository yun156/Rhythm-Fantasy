#pragma once
#include <stddef.h>

typedef struct {
    int   n_notes;
    int  *time_ms;
    int  *lane;
    char  title[128];
    int   length_ms;
    int   bpm;
    char  wav[256];
} BeatMap;

BeatMap* beatmap_load(const char *path);
void beatmap_free(BeatMap *bm);



