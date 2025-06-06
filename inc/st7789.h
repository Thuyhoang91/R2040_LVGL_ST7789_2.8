#ifndef ST7789_DRIVER_H
#define ST7789_DRIVER_H

#include "pico/stdlib.h"
#include "hardware/spi.h"

// Pin Definitions (adjust to your wiring)
#define SPI_PORT spi0
#define PIN_SPI_SCK  18
#define PIN_SPI_MOSI 19
#define PIN_SPI_MISO 16 // Optional, not used in this example for write-only
#define PIN_DC       20
#define PIN_CS       17
#define PIN_RST      21
#define PIN_BLK      22 // Backlight control

// Display dimensions
#define ST7789_WIDTH  240
#define ST7789_HEIGHT 320

// ST7789 Commands
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID   0x04
#define ST7789_RDDST   0x09

#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_PTLON   0x12
#define ST7789_NORON   0x13

#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_PTLAR   0x30
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36

#define ST7789_FRMCTR1 0xB1
#define ST7789_FRMCTR2 0xB2
#define ST7789_FRMCTR3 0xB3
#define ST7789_INVCTR  0xB4
#define ST7789_DISSET5 0xB6

#define ST7789_GCTRL   0xB7
#define ST7789_GTADJ   0xB8
#define ST7789_VCOMS   0xBB

#define ST7789_LCMCTRL 0xC0
#define ST7789_IDSET   0xC1
#define ST7789_VDVVRHEN 0xC2
#define ST7789_VRHS    0xC3
#define ST7789_VDVS    0xC4
#define ST7789_VMCTR1  0xC5
#define ST7789_FRCTRL2 0xC6
#define ST7789_CABCCTRL 0xC7

#define ST7789_PWCTRL1  0xD0
#define ST7789_PVGAMCTRL 0xE0
#define ST7789_NVGAMCTRL 0xE1


void st7789_init();
void st7789_write_cmd(uint8_t cmd);
void st7789_write_data(const uint8_t *data, size_t len);
void st7789_write_data_byte(uint8_t data);
void st7789_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
void st7789_fill_color(uint16_t color, uint32_t len); // For testing
void st7789_set_backlight(uint8_t brightness_percent); // 0-100
void st7789_send_pixels(const uint16_t* pixels, size_t len);

#endif // ST7789_DRIVER_H
