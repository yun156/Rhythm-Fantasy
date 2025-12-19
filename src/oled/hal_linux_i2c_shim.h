#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { int fd; } I2C_HandleTypeDef;

int  MX_I2C1_Init(I2C_HandleTypeDef* h, const char* dev);
int  HAL_I2C_Master_Transmit(I2C_HandleTypeDef* h, uint16_t addr8,
                             const uint8_t* data, uint16_t len, uint32_t timeout_ms);
bool linux_i2c_init(I2C_HandleTypeDef* h);
bool linux_i2c_init_dev(I2C_HandleTypeDef* h, const char* dev);
void HAL_Delay(uint32_t ms);
void linux_i2c_close(I2C_HandleTypeDef* h);

#ifdef __cplusplus
}
#endif



