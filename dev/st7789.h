#ifndef ST7789_H
#define sT7789_h

#include "lvgl.h"

#define PIN_CS 22
#define PIN_RST 21
#define PIN_DC 20
#define PIN_MOSI 19
#define PIN_SCK 18
#define SPI_PORT spi0
#define SPI_BAUD 62.5*1000*1000 // 62.5 MHz
#define ST7789_WIDTH 240
#define ST7789_HEIGHT 320

void st7789_init();
void st7789_flush_dma(lv_disp_t *disp, const lv_area_t *area, uint8_t *color_p);
//void dma_init();

#endif // ST7789_H