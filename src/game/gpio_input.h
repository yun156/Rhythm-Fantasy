#pragma once
#include <stdbool.h>
#include <gpiod.h>

typedef struct {
    struct gpiod_chip *chip;
    struct gpiod_line_request *req;
    int left_off, right_off, hit_off;
} gpio_t;

bool gpio_init(gpio_t *g, int left_off, int right_off, int hit_off);
void gpio_close(gpio_t *g);
int gpio_poll_inputs(gpio_t *g, int timeout_ms, bool *left, bool *right, bool *hit);
int gpio_read_now(gpio_t *g, bool *left, bool *right, bool *hit);



