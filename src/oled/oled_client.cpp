#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

extern "C" void oled_send_score_combo(unsigned int score, unsigned int combo) {
    int fd = open("/tmp/rf_oled.pipe", O_WRONLY | O_NONBLOCK);
    if (fd < 0) return;
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "SCORE %u COMBO %u\n", score, combo);
    if (n > 0) (void)write(fd, buf, n);
    close(fd);
}



