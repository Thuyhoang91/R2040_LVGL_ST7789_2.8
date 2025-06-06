#ifndef XPT2046_DRIVER_H
#define XPT2046_DRIVER_H

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdbool.h>

// Pin Definitions (adjust to your wiring for SPI1)
#define XPT_SPI_PORT spi1
#define PIN_XPT_SCK  10
#define PIN_XPT_MOSI 11
#define PIN_XPT_MISO 12
#define PIN_XPT_CS  13
#define PIN_XPT_IRQ  14  // Touch interrupt (pen down)

// XPT2046 Control Bytes (12-bit conversion, differential reference)
#define XPT2046_CMD_READ_X  0xD0 // Or 0xD1, 0xD2, 0xD3 for specific power/reference modes
#define XPT2046_CMD_READ_Y  0x90 // Or 0x91, 0x92, 0x93
#define XPT2046_CMD_READ_Z1 0xB0
#define XPT2046_CMD_READ_Z2 0xC0

// Calibration values - YOU MUST DETERMINE THESE FOR YOUR SCREEN & ORIENTATION
// These are raw ADC values from XPT2046 corresponding to screen corners.
// Print raw values in your read function to determine these.
// Example placeholders (for a 240x320 screen, adjust based on your findings):
#define XPT2046_MIN_RAW_X 300   // Raw ADC value for screen X=0
#define XPT2046_MAX_RAW_X 3800  // Raw ADC value for screen X=239
#define XPT2046_MIN_RAW_Y 200   // Raw ADC value for screen Y=0
#define XPT2046_MAX_RAW_Y 3700  // Raw ADC value for screen Y=319

// Threshold for touch detection (for Z pressure, if used, or based on IRQ)
#define XPT2046_TOUCH_THRESHOLD 100 // Example pressure threshold

void xpt2046_init(void);
bool xpt2046_is_touched(void);
bool xpt2046_get_touch_point(uint16_t *x, uint16_t *y); // Gets calibrated screen coordinates
bool xpt2046_get_raw_touch_point(uint16_t *x, uint16_t *y, uint16_t *z); // Gets raw ADC values

#endif // XPT2046_DRIVER_H
