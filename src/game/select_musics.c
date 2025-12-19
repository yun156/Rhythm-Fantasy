#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include "common.h"

#define MAX_MUSIC_FILES 100

static char music_files[MAX_MUSIC_FILES][256];
static int music_file_count = 0;
static char selected_song_path[512];

static void join_path(char *out, size_t outsz, const char *base, const char *name) {
    size_t blen = strlen(base);
    if (blen > 0 && base[blen - 1] == '/')
        snprintf(out, outsz, "%s%s", base, name);
    else
        snprintf(out, outsz, "%s/%s", base, name);
}

static int has_wav_ext(const char *fn) {
    const char *dot = strrchr(fn, '.');
    if (!dot) return 0;
    return strcasecmp(dot, ".wav") == 0;
}

static int cmp_music_names(const void *a, const void *b) {
    const char (*sa)[256] = (const char (*)[256])a;
    const char (*sb)[256] = (const char (*)[256])b;
    return strcmp(*sa, *sb);
}

static int already_listed(const char *name) {
    for (int i = 0; i < music_file_count; i++) {
        if (strcmp(music_files[i], name) == 0) return 1;
    }
    return 0;
}

static void list_music_files(void) {
    music_file_count = 0;
    DIR *dir = opendir(MUSIC_DIR);
    if (!dir) {
        fprintf(stderr, "opendir failed: %s (MUSIC_DIR='%s')\n", strerror(errno), MUSIC_DIR);
        return;
    }
    struct dirent *entry;
    struct stat st;
    char fullpath[512];
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (!has_wav_ext(entry->d_name)) continue;
        join_path(fullpath, sizeof(fullpath), MUSIC_DIR, entry->d_name);
        if (stat(fullpath, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (already_listed(entry->d_name)) continue;
        strncpy(music_files[music_file_count], entry->d_name, sizeof(music_files[0]) - 1);
        music_files[music_file_count][sizeof(music_files[0]) - 1] = '\0';
        music_file_count++;
        if (music_file_count >= MAX_MUSIC_FILES) break;
    }
    closedir(dir);
    if (music_file_count > 1)
        qsort(music_files, music_file_count, sizeof(music_files[0]), cmp_music_names);
}

const char* select_music(void) {
    initscr();
    noecho();
    curs_set(0);
    clear();
    list_music_files();
    if (music_file_count == 0) {
        endwin();
        fprintf(stderr, "No .wav files found in %s\n", MUSIC_DIR);
        return NULL;
    }
    for (int i = 0; i < music_file_count; i++) {
        mvprintw(i, 0, "%2d. %s", i + 1, music_files[i]);
    }
    mvprintw(music_file_count + 1, 0, "Select music by number: ");
    refresh();
    int choice = 0;
    echo();
    if (scanw("%d", &choice) != 1) {
        noecho();
        endwin();
        fprintf(stderr, "Input error\n");
        return NULL;
    }
    noecho();
    endwin();
    if (choice < 1 || choice > music_file_count) {
        fprintf(stderr, "Invalid selection\n");
        return NULL;
    }
    join_path(selected_song_path, sizeof(selected_song_path), MUSIC_DIR, music_files[choice - 1]);
    return selected_song_path;
}

void generate_beatmap_path(const char *song_path, char *beatmap_path, size_t size) {
    const char *filename = strrchr(song_path, '/');
#ifdef _WIN32
    const char *b2 = strrchr(song_path, '\\');
    if (!filename || (b2 && b2 > filename)) filename = b2;
#endif
    filename = filename ? filename + 1 : song_path;
    char base[256];
    strncpy(base, filename, sizeof(base) - 1);
    base[sizeof(base) - 1] = '\0';
    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';
    char fname_map[300];
    snprintf(fname_map, sizeof(fname_map), "%s.map", base);
    join_path(beatmap_path, size, MUSIC_DIR, fname_map);
}



