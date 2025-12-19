#include <stdio.h>
#include <string.h>
#include <gpiod.h>
#include "gpio_input.h"

#ifndef GPIOCHIP_PATH
#define GPIOCHIP_PATH "/dev/gpiochip0"
#endif

bool gpio_init(gpio_t *g, int left_off, int right_off, int hit_off) {
    memset(g, 0, sizeof(*g));
    g->left_off = left_off;
    g->right_off = right_off;
    g->hit_off = hit_off;

    unsigned int offsets[3];

    g->chip = gpiod_chip_open(GPIOCHIP_PATH);
    if (!g->chip) { perror("gpiod_chip_open"); return false; }

    struct gpiod_request_config *rcfg = gpiod_request_config_new();
    if (!rcfg) { perror("gpiod_request_config_new"); goto fail_chip; }
    gpiod_request_config_set_consumer(rcfg, "rhythm");

    struct gpiod_line_settings *ls = gpiod_line_settings_new();
    if (!ls) { perror("gpiod_line_settings_new"); goto fail_rcfg; }
    gpiod_line_settings_set_direction(ls, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(ls, GPIOD_LINE_BIAS_PULL_UP);
    gpiod_line_settings_set_edge_detection(ls, GPIOD_LINE_EDGE_BOTH);

    struct gpiod_line_config *lcfg = gpiod_line_config_new();
    if (!lcfg) { perror("gpiod_line_config_new"); goto fail_ls; }

    offsets[0] = (unsigned)g->left_off;
    offsets[1] = (unsigned)g->right_off;
    offsets[2] = (unsigned)g->hit_off;

    if (gpiod_line_config_add_line_settings(lcfg, offsets, 3, ls) < 0) {
        perror("gpiod_line_config_add_line_settings");
        goto fail_lcfg;
    }

    g->req = gpiod_chip_request_lines(g->chip, rcfg, lcfg);
    if (!g->req) { perror("gpiod_chip_request_lines"); goto fail_lcfg; }

    gpiod_line_config_free(lcfg);
    gpiod_line_settings_free(ls);
    gpiod_request_config_free(rcfg);
    return true;

fail_lcfg:
    gpiod_line_config_free(lcfg);
fail_ls:
    gpiod_line_settings_free(ls);
fail_rcfg:
    gpiod_request_config_free(rcfg);
fail_chip:
    gpiod_chip_close(g->chip);
    g->chip = NULL;
    return false;
}

int gpio_poll_inputs(gpio_t *g, int timeout_ms, bool *left, bool *right, bool *hit) {
    if (left) *left = false;
    if (right) *right = false;
    if (hit) *hit = false;

    int64_t timeout_ns = (int64_t)timeout_ms * 1000000LL;
    int wait = gpiod_line_request_wait_edge_events(g->req, timeout_ns);
    if (wait <= 0) return wait;

    struct gpiod_edge_event_buffer *buf = gpiod_edge_event_buffer_new(16);
    if (!buf) { perror("gpiod_edge_event_buffer_new"); return -1; }

    int n = gpiod_line_request_read_edge_events(g->req, buf, 16);
    if (n < 0) { perror("gpiod_line_request_read_edge_events"); n = -1; }

    for (int i = 0; i < n; ++i) {
        struct gpiod_edge_event *ev = gpiod_edge_event_buffer_get_event(buf, i);
        int off = gpiod_edge_event_get_line_offset(ev);
        int type = gpiod_edge_event_get_event_type(ev);
        bool falling = (type == GPIOD_EDGE_EVENT_FALLING_EDGE);
        if (falling) {
            if (off == g->left_off && left) *left = true;
            if (off == g->right_off && right) *right = true;
            if (off == g->hit_off && hit) *hit = true;
        }
    }

    gpiod_edge_event_buffer_free(buf);
    return n;
}

int gpio_read_now(gpio_t *g, bool *left, bool *right, bool *hit) {
    int v;
    if (left) { v = gpiod_line_request_get_value(g->req, g->left_off); if (v < 0) return -1; *left = (v == 0); }
    if (right){ v = gpiod_line_request_get_value(g->req, g->right_off); if (v < 0) return -1; *right = (v == 0); }
    if (hit)  { v = gpiod_line_request_get_value(g->req, g->hit_off); if (v < 0) return -1; *hit = (v == 0); }
    return 0;
}

void gpio_close(gpio_t *g) {
    if (!g) return;
    if (g->req) { gpiod_line_request_release(g->req); g->req = NULL; }
    if (g->chip) { gpiod_chip_close(g->chip); g->chip = NULL; }
}



