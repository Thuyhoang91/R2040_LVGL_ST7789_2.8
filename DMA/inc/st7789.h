#ifndef ST7789_DRIVE_H
#define ST7789_DRIVE_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lvgl.h"

// Define your pins
#define LCD_PIN_DC      20
#define LCD_PIN_RST     21
#define LCD_PIN_BLK     22
#define SPI_PORT        spi0
#define SPI_PIN_CS      17
#define SPI_PIN_SCK     18
#define SPI_PIN_MOSI    19

void st7789_init(uint8_t madctl_val);
void st7789_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
void st7789_write_data_dma(const void *data, size_t len);
bool st7789_dma_is_busy();

#endif
