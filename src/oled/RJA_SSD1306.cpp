// RJA_SSD1306.cpp
#include "RJA_SSD1306.h"

constexpr unsigned char OLED::font1[];

OLED::OLED() {}
OLED::~OLED() {}

void OLED::init(I2C_HandleTypeDef *hi2c1)
{
    hi2cI = hi2c1;

    uint8_t b2[2];
    b2[0] = 0x00; b2[1] = 0xAE; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
    b2[0] = 0x00; b2[1] = 0x20; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
    b2[0] = 0x00; b2[1] = 0x00; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
    b2[0] = 0x00; b2[1] = 0x40; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
    b2[0] = 0x00; b2[1] = 0x8D; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
    b2[0] = 0x00; b2[1] = 0x14; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
    b2[0] = 0x00; b2[1] = 0xA4; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
    b2[0] = 0x00; b2[1] = 0xAF; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            frame[x][y] = 0;

    setOrientation(true, true);
    setDisplayOffset(8);
    setStartLine(0);
}

void OLED::drawFullscreen()
{
    uint8_t cmds[] = {
        0x00, 0x20, 0x00,
        0x00, 0x21, 0x00, 0x7F,
        0x00, 0x22, 0x00, 0x07
    };
    HAL_I2C_Master_Transmit(hi2cI, CAddress, cmds, sizeof(cmds), 10);

    uint8_t pkt[1 + 16];
    pkt[0] = 0x40;

    for (int page = 0; page < 8; ++page) {
        for (int col = 0; col < 128; col += 16) {
            for (int k = 0; k < 16; ++k) {
                uint8_t b = 0;
                int x = col + k;
                for (int bit = 0; bit < 8; ++bit) {
                    int y = page * 8 + bit;
                    b |= (frame[x][y] ? 1 : 0) << bit;
                }
                pkt[1 + k] = b;
            }
            HAL_I2C_Master_Transmit(hi2cI, CAddress, pkt, sizeof(pkt), 10);
        }
    }
}

void OLED::setContrast(uint8_t contrast)
{
    uint8_t b2[2];
    b2[0] = 0x00; b2[1] = 0x81; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 5);
    b2[0] = 0x00; b2[1] = contrast; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 5);
}

void OLED::setOrientation(bool flip_h, bool flip_v)
{
    uint8_t b2[2];
    b2[0] = 0x00; b2[1] = flip_h ? 0xA1 : 0xA0; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
    b2[0] = 0x00; b2[1] = flip_v ? 0xC8 : 0xC0; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
}

void OLED::setDisplayOffset(uint8_t ofs)
{
    uint8_t b2[2];
    b2[0] = 0x00; b2[1] = 0xD3; HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
    b2[0] = 0x00; b2[1] = (uint8_t)(ofs & 0x3F); HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
}

void OLED::setStartLine(uint8_t line)
{
    uint8_t b2[2];
    b2[0] = 0x00; b2[1] = (uint8_t)(0x40 | (line & 0x3F)); HAL_I2C_Master_Transmit(hi2cI, CAddress, b2, 2, 10);
}

void OLED::pixel(int x, int y, bool colour, bool locked)
{
    if (locked) {
        if (x < 0 || y < 0 || x >= width || y >= height) return;
    }
    frame[x][y] = colour;
}

void OLED::fill(bool colour)
{
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            pixel(x, y, colour, true);
}

void OLED::character(int x, int y, unsigned char c, bool colour, bool bg, int size)
{
    if ((x >= width) || (y >= height) || ((x + 6 * size - 1) < 0) || ((y + 8 * size - 1) < 0))
        return;
    if (!_cp437 && (c >= 176)) c++;

    for (int8_t i = 0; i < 6; i++) {
        uint8_t line = (i == 5) ? 0x0 : pgm_read_byte(&font1[(c * 5) + i]);
        for (int8_t j = 0; j < 8; j++) {
            if (line & 0x1) {
                if (size == 1) {
                    pixel(x + i, y + j, colour, 1);
                } else {
                    int x0 = x + i * size;
                    int y0 = y + j * size;
                    for (int yy = 0; yy < size; ++yy)
                        for (int xx = 0; xx < size; ++xx)
                            pixel(x0 + xx, y0 + yy, colour, 1);
                }
            } else if (bg != colour) {
                if (size == 1) {
                    pixel(x + i, y + j, bg, 1);
                } else {
                    int x0 = x + i * size;
                    int y0 = y + j * size;
                    for (int yy = 0; yy < size; ++yy)
                        for (int xx = 0; xx < size; ++xx)
                            pixel(x0 + xx, y0 + yy, bg, 1);
                }
            }
            line >>= 1;
        }
    }
}

void OLED::text(int x, int y, string s, bool colour, bool bg, int size)
{
    int offset = size * 6;
    for (string::size_type i = 0; i < s.size(); i++)
        character(x + (offset * (int)i), y, (unsigned char)s[i], colour, bg, size);
}
