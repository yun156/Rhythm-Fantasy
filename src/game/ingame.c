#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <locale.h>
#include <limits.h>
#include <stdlib.h>

#include "ingame.h"
#include "beatmap.h"
#include "gpio_input.h"
#include "common.h"

#define FPS 60
#define GREAT_MS 85
#define GOOD_MS 160
#define TRAVEL_MS 2000
#define HIT_ROW_MARGIN 3
#define FX_MS 300

void oled_send_score_combo(unsigned int score, unsigned int combo);

static inline int64_t now_ms(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec*1000 + ts.tv_nsec/1000000;
}

static inline void push_oled_maybe(const GameResult* gr, int64_t now){
    static unsigned last_score = UINT_MAX, last_combo = UINT_MAX;
    static int64_t next_ok_ms = 0;
    if ((gr->score != (int)last_score || gr->combo != (int)last_combo) && now >= next_ok_ms) {
        oled_send_score_combo((unsigned)gr->score, (unsigned)gr->combo);
        last_score = (unsigned)gr->score;
        last_combo = (unsigned)gr->combo;
        next_ok_ms = now + 33;
    }
}

static void ui_init(void){
    setlocale(LC_ALL, "");
    initscr(); cbreak(); noecho(); nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE); curs_set(0); start_color(); use_default_colors();
    init_pair(1, COLOR_WHITE, -1);
    init_pair(9, COLOR_GREEN, -1);
    init_pair(10, COLOR_RED, -1);
    init_pair(11, COLOR_BLUE, -1);
    init_pair(12, COLOR_YELLOW, -1);
}

static void ui_end(void){ endwin(); }

static void draw_header(const char *title, const char *nick, int score, int combo, int great, int good, int miss){
    attron(A_BOLD);
    mvprintw(0, 2, "Song: %s", (title && *title) ? title : "(unknown)");
    mvprintw(1, 2, "Player: %s", nick ? nick : "PLAYER");
    attroff(A_BOLD);
    mvprintw(0, 40, "Score: %6d  Combo: %3d", score, combo);
    mvprintw(1, 40, "Great:%3d  Good:%3d  Miss:%3d", great, good, miss);
}

static void lane_xs(int cols, int x[3]){
    x[0] = cols/4; x[1] = cols/2; x[2] = (3*cols)/4;
}

static void draw_lanes(int rows, int cols){
    int x[3]; lane_xs(cols, x);
    int hit_row = rows - HIT_ROW_MARGIN;
    attron(COLOR_PAIR(1));
    for (int i=0;i<3;i++){
        for (int y=2;y<rows-1;y++) mvaddch(y, x[i], ACS_VLINE);
    }
    for (int c=2;c<cols-2;c++) mvaddch(hit_row, c, ACS_HLINE);
    attroff(COLOR_PAIR(1));
}

static void draw_note(int lane, int y, int rows, int cols){
    int x[3]; lane_xs(cols, x);
    if (lane<0 || lane>2) return; if (y<2 || y>=rows-1) return;
    attron(COLOR_PAIR(9) | A_BOLD); mvaddch(y, x[lane], '@'); attroff(COLOR_PAIR(9) | A_BOLD);
}

typedef struct { int lane; int time_ms; bool hit; } LiveNote;
typedef struct { LiveNote *arr; int n; } LiveChart;

static void build_live_chart(const BeatMap *bm, LiveChart *lc){
    lc->n = bm->n_notes;
    lc->arr = (LiveNote*)calloc((size_t)lc->n, sizeof(LiveNote));
    for (int i=0;i<lc->n;i++){
        lc->arr[i].lane = bm->lane[i];
        lc->arr[i].time_ms = bm->time_ms[i];
        lc->arr[i].hit = false;
    }
}
static void free_live_chart(LiveChart *lc){ free(lc->arr); lc->arr=NULL; lc->n=0; }

static int judge_from_diff(int diff_ms){
    int ad = diff_ms<0 ? -diff_ms : diff_ms;
    if (ad <= GREAT_MS) return 2;
    if (ad <= GOOD_MS) return 1;
    return 0;
}

typedef struct { int row, col; char glyph; int color_pair; int64_t until_ms; bool alive; } Fx;
#define MAX_FX 64
static Fx g_fx[MAX_FX];
static void fx_clear_all(void){ memset(g_fx, 0, sizeof(g_fx)); }
static void spawn_fx(int row, int col, int lane, char glyph, int64_t now){
    int color = (lane==0)?10:(lane==1)?11:12;
    for (int i=0;i<MAX_FX;i++) if(!g_fx[i].alive){
        g_fx[i].row=row; g_fx[i].col=col; g_fx[i].glyph=glyph;
        g_fx[i].color_pair=color; g_fx[i].until_ms=now+FX_MS; g_fx[i].alive=true; return;
    }
}
static void draw_fx_and_gc(int rows, int cols, int64_t now){
    (void)rows; (void)cols;
    for (int i=0;i<MAX_FX;i++){
        if(!g_fx[i].alive) continue;
        if (now >= g_fx[i].until_ms) { g_fx[i].alive=false; continue; }
        attron(COLOR_PAIR(g_fx[i].color_pair) | A_BOLD);
        mvaddch(g_fx[i].row, g_fx[i].col, g_fx[i].glyph);
        attroff(COLOR_PAIR(g_fx[i].color_pair) | A_BOLD);
    }
}

static int pick_lowest_visible_note(const LiveChart *lc, int lane, int elapsed, int rows, int cols, int *out_y){
    int x[3]; lane_xs(cols, x); (void)x;
    int hit_row = rows - HIT_ROW_MARGIN;
    int best_idx=-1, best_y=-1;
    for (int i=0;i<lc->n;i++){
        if (lc->arr[i].hit || lc->arr[i].lane != lane) continue;
        int appear = lc->arr[i].time_ms - TRAVEL_MS;
        int end = lc->arr[i].time_ms;
        if (elapsed < appear - 300) continue;
        if (elapsed > end + 400) continue;
        double prog = (double)(elapsed - appear) / (double)TRAVEL_MS;
        if (prog < 0.0) prog = 0.0; if (prog > 1.0) prog = 1.0;
        int y = 2 + (int)(prog * (double)(hit_row - 2));
        if (y > best_y) { best_y=y; best_idx=i; }
    }
    if (out_y) *out_y = (best_idx>=0) ? best_y : hit_row;
    return best_idx;
}

GameResult play_game_ncurses(BeatMap *bm, gpio_t *gpio, const char *nick){
    GameResult gr; memset(&gr, 0, sizeof(gr));
    ui_init();
    fx_clear_all();
    oled_send_score_combo(0, 0);
    int rows, cols; getmaxyx(stdscr, rows, cols);
    int hit_row = rows - HIT_ROW_MARGIN;
    int lane_x[3]; lane_xs(cols, lane_x);
    LiveChart lc = (LiveChart){0}; build_live_chart(bm, &lc);
    for (int i=3;i>=1;i--){
        erase(); draw_header(bm->title, nick, gr.score, gr.combo, gr.great, gr.good, gr.miss);
        mvprintw(rows/2, (cols/2)-8, "Starting in %d...", i);
        refresh(); usleep(500*1000);
    }
    int64_t t0 = now_ms();
    const int frame_us = 1000000 / FPS;
    while (1){
        int64_t tnow = now_ms();
        int elapsed = (int)(tnow - t0);
        bool L=false, R=false, H=false;
        (void)gpio_poll_inputs(gpio, 0, &L, &R, &H);
        int ch = getch();
        if (ch=='q' || ch=='Q') break;
        if (L || R || H){
            int lanes_pressed[3] = { L?1:0, R?1:0, H?1:0 };
            for (int lane=0; lane<3; lane++){
                if (!lanes_pressed[lane]) continue;
                int y_hit=hit_row;
                int idx = pick_lowest_visible_note(&lc, lane, elapsed, rows, cols, &y_hit);
                if (idx >= 0){
                    int diff = elapsed - lc.arr[idx].time_ms;
                    int j = judge_from_diff(diff);
                    if (j == 2){
                        gr.great++; gr.combo++; if (gr.combo>gr.max_combo) gr.max_combo=gr.combo;
                        gr.score += 2; lc.arr[idx].hit = true; spawn_fx(y_hit, lane_x[lane], lane, 'A', tnow);
                    } else if (j == 1){
                        gr.good++; gr.combo++; if (gr.combo>gr.max_combo) gr.max_combo=gr.combo;
                        gr.score += 1; lc.arr[idx].hit = true; spawn_fx(y_hit, lane_x[lane], lane, 'B', tnow);
                    } else {
                        gr.miss++; gr.score += gr.combo; gr.combo = 0;
                        lc.arr[idx].hit = true; spawn_fx(y_hit, lane_x[lane], lane, 'X', tnow);
                    }
                } else {
                    gr.miss++; gr.score += gr.combo; gr.combo = 0;
                    spawn_fx(hit_row, lane_x[lane], lane, 'X', tnow);
                }
            }
        }
        for (int i=0;i<lc.n;i++){
            if (lc.arr[i].hit) continue;
            if (elapsed > lc.arr[i].time_ms + GOOD_MS){
                int lane = lc.arr[i].lane;
                lc.arr[i].hit = true;
                gr.miss++; gr.score += gr.combo; gr.combo = 0;
                spawn_fx(hit_row, lane_x[lane], lane, 'X', tnow);
            }
        }
        push_oled_maybe(&gr, tnow);
        erase();
        draw_header(bm->title, nick, gr.score, gr.combo, gr.great, gr.good, gr.miss);
        draw_lanes(rows, cols);
        int visible = 0;
        for (int i=0;i<lc.n;i++){
            if (lc.arr[i].hit) continue;
            int appear = lc.arr[i].time_ms - TRAVEL_MS;
            int end = lc.arr[i].time_ms;
            if (elapsed < appear - 300) continue;
            if (elapsed > end + 400) continue;
            double prog = (double)(elapsed - appear) / (double)TRAVEL_MS;
            if (prog < 0.0) prog = 0.0; if (prog > 1.0) prog = 1.0;
            int y = 2 + (int)(prog * (double)(hit_row - 2));
            draw_note(lc.arr[i].lane, y, rows, cols);
            visible++;
        }
        draw_fx_and_gc(rows, cols, tnow);
        int finished = 1;
        for (int i=0;i<lc.n;i++) if (!lc.arr[i].hit){ finished=0; break; }
        if (!visible && finished){ refresh(); usleep(200*1000); break; }
        refresh();
        usleep(frame_us);
    }
    oled_send_score_combo((unsigned)gr.score, (unsigned)gr.combo);
    erase();
    attron(A_BOLD); mvprintw(rows/2-1, (cols/2)-6, "=== RESULT ==="); attroff(A_BOLD);
    mvprintw(rows/2+0, (cols/2)-10, "Score: %d, MaxCombo: %d", gr.score, gr.max_combo);
    mvprintw(rows/2+1, (cols/2)-10, "Great: %d, Good: %d, Miss: %d", gr.great, gr.good, gr.miss);
    mvprintw(rows/2+3, (cols/2)-12, "Press any key to continue...");
    nodelay(stdscr, FALSE); getch();
    ui_end();
    free_live_chart(&lc);
    return gr;
}



