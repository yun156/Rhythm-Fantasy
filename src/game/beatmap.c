#include "beatmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#define INIT_CAP 128

static const int *g_sort_times = NULL;
static int cmp_idx(const void *A, const void *B){
    int i = *(const int*)A, j = *(const int*)B;
    return (g_sort_times[i] > g_sort_times[j]) - (g_sort_times[i] < g_sort_times[j]);
}

static void *xrealloc(void *p, size_t nbytes){
    void *q = realloc(p, nbytes);
    if (!q) free(p);
    return q;
}

static char *ltrim(char *s){
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static void rtrim(char *s){
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}

static const char* basename_noext(const char *path){
    const char *b = strrchr(path, '/');
#ifdef _WIN32
    const char *b2 = strrchr(path, '\\');
    if (!b || (b2 && b2 > b)) b = b2;
#endif
    b = b ? b + 1 : path;
    static char buf[256];
    snprintf(buf, sizeof(buf), "%s", b);
    char *dot = strrchr(buf, '.');
    if (dot) *dot = '\0';
    return buf;
}

static void sort_notes(BeatMap *bm){
    if (bm->n_notes <= 1) return;
    int n = bm->n_notes;
    int *idx = (int*)malloc(sizeof(int)*n);
    if (!idx) return;
    for (int i=0;i<n;i++) idx[i]=i;
    g_sort_times = bm->time_ms;
    qsort(idx, n, sizeof(int), cmp_idx);
    g_sort_times = NULL;
    int *t2 = (int*)malloc(sizeof(int)*n);
    int *l2 = (int*)malloc(sizeof(int)*n);
    if (!t2 || !l2){ free(t2); free(l2); free(idx); return; }
    for (int k=0;k<n;k++){ t2[k]=bm->time_ms[idx[k]]; l2[k]=bm->lane[idx[k]]; }
    memcpy(bm->time_ms, t2, sizeof(int)*n);
    memcpy(bm->lane,    l2, sizeof(int)*n);
    free(t2); free(l2); free(idx);
}

BeatMap* beatmap_load(const char *path){
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;

    BeatMap *bm = (BeatMap*)calloc(1, sizeof(*bm));
    if (!bm){ fclose(fp); return NULL; }

    int cap = INIT_CAP;
    bm->time_ms = (int*)malloc(sizeof(int) * cap);
    bm->lane    = (int*)malloc(sizeof(int) * cap);
    if (!bm->time_ms || !bm->lane){
        beatmap_free(bm);
        fclose(fp);
        return NULL;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)){
        char *p = ltrim(line);
        rtrim(p);
        if (*p == '\0') continue;
        if (p[0] == ';' || (p[0]=='/' && p[1]=='/')) continue;
        if (strncasecmp(p, "TITLE:", 6) == 0){
            p = ltrim(p+6);
            snprintf(bm->title, sizeof(bm->title), "%s", p);
            continue;
        }
        if (strncasecmp(p, "BPM:", 4) == 0){
            int v=0; if (sscanf(p+4, "%d", &v)==1) bm->bpm = v;
            continue;
        }
        if (p[0] == '#'){
            int v=0; if (sscanf(p+1, "%d", &v)==1) bm->bpm = v;
            continue;
        }
        if (strncasecmp(p, "@len", 4) == 0){
            int len=0; if (sscanf(p+4, "%d", &len)==1) bm->length_ms = len;
            continue;
        }
        if (strstr(p, ".wav") || strstr(p, ".WAV")){
            if (bm->wav[0] == '\0') snprintf(bm->wav, sizeof(bm->wav), "%s", p);
        }
        int t=0, lane=0;
        if (sscanf(p, "%d %d", &t, &lane) == 2){
            if (1 <= lane && lane <= 3) lane -= 1;
            if (lane < 0) lane = 0;
            if (lane > 2) lane = 2;
            if (bm->n_notes >= cap){
                cap *= 2;
                bm->time_ms = (int*)xrealloc(bm->time_ms, sizeof(int)*cap);
                bm->lane    = (int*)xrealloc(bm->lane,    sizeof(int)*cap);
                if (!bm->time_ms || !bm->lane){ beatmap_free(bm); fclose(fp); return NULL; }
            }
            bm->time_ms[bm->n_notes] = t;
            bm->lane[bm->n_notes]    = lane;
            bm->n_notes++;
            continue;
        }
    }
    fclose(fp);

    if (bm->n_notes == 0){ beatmap_free(bm); return NULL; }
    sort_notes(bm);
    if (bm->length_ms <= 0) bm->length_ms = bm->time_ms[bm->n_notes-1] + 2000;
    if (bm->title[0] == '\0'){
        const char *bn = basename_noext(path);
        snprintf(bm->title, sizeof(bm->title), "%s", bn);
    }
    return bm;
}

void beatmap_free(BeatMap *bm){
    if (!bm) return;
    free(bm->time_ms);
    free(bm->lane);
    free(bm);
}



