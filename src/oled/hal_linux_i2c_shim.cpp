#include "hal_linux_i2c_shim.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

static const char* kDev = "/dev/i2c-1";

extern "C" int MX_I2C1_Init(I2C_HandleTypeDef* h, const char* dev) {
    if (!h || !dev) return -1;
    h->fd = open(dev, O_RDWR);
    return (h->fd >= 0) ? 0 : -1;
}

extern "C" bool linux_i2c_init_dev(I2C_HandleTypeDef* h, const char* dev) {
    return MX_I2C1_Init(h, dev) == 0;
}

extern "C" bool linux_i2c_init(I2C_HandleTypeDef* h) {
    return linux_i2c_init_dev(h, kDev);
}

extern "C" int HAL_I2C_Master_Transmit(I2C_HandleTypeDef* h, uint16_t addr8,
                                       const uint8_t* data, uint16_t len, uint32_t) {
    if (!h || h->fd < 0 || !data) return -1;
    int addr7 = addr8 >> 1;
    if (ioctl(h->fd, I2C_SLAVE, addr7) < 0) return -1;
    ssize_t w = write(h->fd, data, len);
    return (w == (ssize_t)len) ? 0 : -1;
}

extern "C" void HAL_Delay(uint32_t ms) {
    usleep(ms * 1000);
}

extern "C" void linux_i2c_close(I2C_HandleTypeDef* h) {
    if (h && h->fd >= 0) {
        close(h->fd);
        h->fd = -1;
    }
}



