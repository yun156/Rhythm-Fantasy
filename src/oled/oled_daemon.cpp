#include <cstdio>
#include <cstdint>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <cerrno>
#include <time.h>
#include "hal_linux_i2c_shim.h"
#include "RJA_SSD1306.h"

static const char* FIFO_PATH = "/tmp/rf_oled.pipe";

static inline int64_t now_ms(){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec*1000 + ts.tv_nsec/1000000;
}

static int ensure_fifo(const char* path) {
    struct stat st{};
    if (stat(path, &st) == 0) {
        if (!S_ISFIFO(st.st_mode)) return -1;
        return 0;
    }
    if (mkfifo(path, 0666) < 0 && errno != EEXIST) return -1;
    return 0;
}

static int center_x(const std::string& s, int size) {
    int char_w = 6 * size;
    int w = (int)s.size() * char_w;
    int x = (128 - w) / 2;
    if (x < 0) x = 0;
    return x;
}

static void draw_screen(OLED& oled, uint32_t score, uint32_t combo) {
    oled.fill(false);
    std::string title = "RHYTHM FANTASY";
    oled.text(center_x(title, 1), 0, title, true, false, 1);
    std::string s_label = "SCORE";
    std::string s_value = std::to_string(score);
    oled.text(center_x(s_label, 1), 10, s_label, true, false, 1);
    oled.text(center_x(s_value, 2), 20, s_value, true, false, 2);
    std::string c_label = "COMBO";
    std::string c_value = std::to_string(combo);
    oled.text(center_x(c_label, 1), 37, c_label, true, false, 1);
    oled.text(center_x(c_value, 2), 47, c_value, true, false, 2);
    oled.drawFullscreen();
}

int main() {
    if (ensure_fifo(FIFO_PATH) != 0) return 1;
    I2C_HandleTypeDef hi2c{};
    if (!linux_i2c_init(&hi2c)) return 2;
    OLED oled; oled.init(&hi2c);
    uint32_t score = 0, combo = 0;
    draw_screen(oled, score, combo);
    int fd_r = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fd_r < 0) return 3;
    int fd_w_dummy = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    (void)fd_w_dummy;
    std::string buffer; buffer.reserve(256);
    uint32_t pending_score = score, pending_combo = combo;
    bool dirty = false;
    int64_t last_draw = 0;
    const int DRAW_MIN_INTERVAL_MS = 50;
    struct pollfd pfd{fd_r, POLLIN, 0};
    while (true) {
        int pr = poll(&pfd, 1, 20);
        if (pr > 0 && (pfd.revents & POLLIN)) {
            char temp[256];
            ssize_t n = read(fd_r, temp, sizeof(temp));
            if (n > 0) {
                buffer.append(temp, temp + n);
                size_t pos;
                while ((pos = buffer.find('\n')) != std::string::npos) {
                    std::string line = buffer.substr(0, pos);
                    buffer.erase(0, pos + 1);
                    uint32_t sc = pending_score, cb = pending_combo;
                    if (std::sscanf(line.c_str(), "SCORE %u COMBO %u", &sc, &cb) == 2 ||
                        std::sscanf(line.c_str(), "%u %u", &sc, &cb) == 2) {
                        if (sc != pending_score || cb != pending_combo) {
                            pending_score = sc; pending_combo = cb; dirty = true;
                        }
                    }
                }
            } else if (n == 0) {
                close(fd_r);
                fd_r = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
                pfd.fd = fd_r;
            }
        }
        int64_t tnow = now_ms();
        if (dirty && (tnow - last_draw >= DRAW_MIN_INTERVAL_MS)) {
            draw_screen(oled, pending_score, pending_combo);
            last_draw = tnow;
            dirty = false;
        }
    }
    return 0;
}



