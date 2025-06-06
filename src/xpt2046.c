#include "xpt2046.h"
#include "pico/time.h"
#include <stdio.h> // For printf debugging
#include "lvgl.h"  // For LV_HOR_RES_MAX, LV_VER_RES_MAX

// SPI configuration for XPT2046 (typically slower than display)
#define XPT_SPI_BAUD_RATE (2.5 * 1000 * 1000) // 2 MHz, XPT2046 max is often around 2.5MHz

static inline void xpt_cs_select() {
    gpio_put(PIN_XPT_CS, 0);
    // Small delay might be needed for some XPT2046 chips after CS goes low
    // busy_wait_us_32(1); // Adjust if needed
}

static inline void xpt_cs_deselect() {
    gpio_put(PIN_XPT_CS, 1);
}

static uint16_t xpt2046_read_value(uint8_t cmd) {
    uint8_t tx_buf[3] = {cmd, 0x00, 0x00};
    uint8_t rx_buf[3];

    xpt_cs_select();
    spi_write_read_blocking(XPT_SPI_PORT, tx_buf, rx_buf, 3);
    xpt_cs_deselect();

    // The XPT2046 returns a 12-bit value.
    // For commands like 0xD0, 0x90, etc. (12-bit mode), the data is typically
    // in bits D11-D0 across the second and third bytes received.
    // rx_buf[0] is usually ignored or contains status.
    // rx_buf[1] contains the MSBs, rx_buf[2] contains the LSBs.
    // The 12 bits are usually MSB-aligned in these 16 bits, so right shift by 3 or 4.
    // Check your XPT2046 datasheet, common is ((rx_buf[1] << 8) | rx_buf[2]) >> 3
    // or >> 4. For a 0-4095 range, >>3 is typical.
    return ((rx_buf[1] << 8) | rx_buf[2]) >> 3;
}

void xpt2046_init(void) {
    // Initialize GPIO for CS
    gpio_init(PIN_XPT_CS);
    gpio_set_dir(PIN_XPT_CS, GPIO_OUT);
    gpio_put(PIN_XPT_CS, 1); // Deselect

    // Initialize GPIO for IRQ
    gpio_init(PIN_XPT_IRQ);
    gpio_set_dir(PIN_XPT_IRQ, GPIO_IN);
    gpio_pull_up(PIN_XPT_IRQ); // XPT2046 IRQ is often open-drain, active low. Pull-up needed.

    // Initialize SPI1
    spi_init(XPT_SPI_PORT, XPT_SPI_BAUD_RATE);
    gpio_set_function(PIN_XPT_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_XPT_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_XPT_MISO, GPIO_FUNC_SPI);

    // XPT2046 typically uses SPI Mode 0 (CPOL=0, CPHA=0)
    spi_set_format(XPT_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    printf("XPT2046 Initialized on SPI1\n");
}

bool xpt2046_is_touched(void) {
    // IRQ pin is active low when touched
    return gpio_get(PIN_XPT_IRQ) == 0;
}

// Gets RAW ADC values
bool xpt2046_get_raw_touch_point(uint16_t *raw_x, uint16_t *raw_y, uint16_t *raw_z) {
    if (!xpt2046_is_touched()) {
        return false;
    }

    // For more stable readings, one might discard the first reading
    // or average multiple readings.
    // It's also common to power down between reads if not continuously sampling
    // using the PD0/PD1 bits in the command byte.
    // For simplicity here, we do direct reads.

    *raw_x = xpt2046_read_value(XPT2046_CMD_READ_X);
    *raw_y = xpt2046_read_value(XPT2046_CMD_READ_Y);
    if (raw_z) { // Optional Z reading
        *raw_z = xpt2046_read_value(XPT2046_CMD_READ_Z1); // Or combine Z1 and Z2
    }
     printf("RAW_X: %u, RAW_Y: %u\n", *raw_x, *raw_y); // <<< DEBUG PRINT
    // A very basic filter: if X or Y is 0 or 4095 (max ADC value), it might be noise
    // This depends on your calibration range, so adjust if needed.
    if (*raw_x == 0 || *raw_x >= 4095 || *raw_y == 0 || *raw_y >= 4095) {
        // This can happen when lifting the pen or at the very edges.
        // Might indicate an invalid read or edge of sensor.
        // printf("Raw touch potentially noisy: X=%u, Y=%u\n", *raw_x, *raw_y);
        // return false; // Uncomment if you want to discard these noisy readings
    }

    // It's a good idea to read Y again after X or vice versa to ensure the ADC has settled.
    // Some XPT2046 drivers do: Read X, Read Y, Read Y again. Use the second Y.
    // Or even better: Read X, Read Y, Read X, Read Y. Average the X's, average the Y's.

    return true;
}


// Gets CALIBRATED screen coordinates
bool xpt2046_get_touch_point(uint16_t *x, uint16_t *y) {
    uint16_t raw_x, raw_y, raw_z;

    if (!xpt2046_get_raw_touch_point(&raw_x, &raw_y, &raw_z)) {
        return false;
    }

    //--- CALIBRATION AND MAPPING ---
    // This is where you map raw ADC values to screen coordinates.
    // The logic here depends HEAVILY on your display's rotation (MADCTL for ST7789)
    // and the physical orientation of the touch panel relative to the display.

    // Assumed orientation:
    // ST7789 MADCTL = 0x00 (Portrait, 240W x 320H)
    // LV_HOR_RES_MAX = 240, LV_VER_RES_MAX = 320
    // Touch panel oriented such that its X-axis aligns with screen X, Y-axis with screen Y.

    // Ensure raw values are within calibrated range to prevent division by zero or overflow
    if (raw_x < XPT2046_MIN_RAW_X) raw_x = XPT2046_MIN_RAW_X;
    if (raw_x > XPT2046_MAX_RAW_X) raw_x = XPT2046_MAX_RAW_X;
    if (raw_y < XPT2046_MIN_RAW_Y) raw_y = XPT2046_MIN_RAW_Y;
    if (raw_y > XPT2046_MAX_RAW_Y) raw_y = XPT2046_MAX_RAW_Y;

    int32_t cal_x, cal_y;

    // Check for swapped axes or inverted axes based on calibration values.
    // This example assumes MIN_RAW < MAX_RAW. If your MAX_RAW is smaller than MIN_RAW,
    // it means that axis is inverted.

    // Standard mapping (if XPT_MIN_RAW_X < XPT_MAX_RAW_X)
    cal_x = LV_HOR_RES_MAX - ( (int32_t)raw_x - XPT2046_MIN_RAW_X ) * LV_HOR_RES_MAX / (XPT2046_MAX_RAW_X - XPT2046_MIN_RAW_X);
    // If X-axis is inverted (e.g. MAX_RAW_X < MIN_RAW_X from calibration)
    //cal_x = (XPT2046_MIN_RAW_X - (int32_t)raw_x ) * LV_HOR_RES_MAX / (XPT2046_MIN_RAW_X - XPT2046_MAX_RAW_X);
    // or simpler: cal_x = LV_HOR_RES_MAX - ( ( (int32_t)raw_x - XPT_MAX_RAW_X ) * LV_HOR_RES_MAX / (XPT_MIN_RAW_X - XPT_MAX_RAW_X) );

    // Standard mapping (if XPT_MIN_RAW_Y < XPT_MAX_RAW_Y)
    cal_y = ( (int32_t)raw_y - XPT2046_MIN_RAW_Y ) * LV_VER_RES_MAX / (XPT2046_MAX_RAW_Y - XPT2046_MIN_RAW_Y);
    // If Y-axis is inverted
    // cal_y = ( (XPT2046_MIN_RAW_Y - (int32_t)raw_y) * LV_VER_RES_MAX / (XPT2046_MIN_RAW_Y - XPT2046_MAX_RAW_Y) );

    // --- Handle Screen Rotation (ST7789 MADCTL & LVGL Resolution) ---
    // This section is crucial if your display is rotated relative to the touch panel's native axes.
    // `madctl_val` from st7789_driver.c will determine this.
    // Example: uint8_t madctl_val = 0x00; // In st7789_driver.c
    // Let's assume `LV_HOR_RES_MAX` and `LV_VER_RES_MAX` in `lv_conf.h` are set to the
    // *final logical dimensions* after rotation.
    //
    // If MADCTL's MV bit (bit 5) is set (row/column exchange for ST7789):
    //   The ST7789's idea of X and Y are swapped. LVGL's HOR_RES/VER_RES will match this.
    //   Your touch panel's X might now map to the screen's Y, and touch Y to screen X.
    //
    // Example scenarios (assuming LV_HOR_RES_MAX=240, LV_VER_RES_MAX=320 for portrait):
    //   1. madctl_val = 0x00 (Portrait, no ST7789 rotation)
    //      *x = cal_x; *y = cal_y;
    //
    //   2. madctl_val = 0x60 (MX | MV -> Landscape, 320W x 240H)
    //      LV_HOR_RES_MAX should be 320, LV_VER_RES_MAX 240 in lv_conf.h
    //      Touch X maps to Screen Y, Touch Y maps to Screen X (possibly inverted)
    //      *x = cal_y; // raw Y becomes screen X
    //      *y = LV_VER_RES_MAX - cal_x; // raw X becomes screen Y, inverted
    //      (This requires careful calibration of XPT2046_MIN/MAX values for this orientation)

    // For simplicity, we'll assume no rotation or the calibration values
    // XPT2046_MIN/MAX_RAW_X/Y are ALREADY determined for the final screen orientation.
    // If ST7789 MADCTL has MV (bit 5) set, you might need to swap cal_x and cal_y here
    // or adjust which raw axis maps to which calibrated axis.

    // Default: No rotation assumed beyond basic calibration
    *x = (uint16_t)cal_x;
    *y = (uint16_t)cal_y;


    // Clamp to screen boundaries
    if (*x >= LV_HOR_RES_MAX) *x = LV_HOR_RES_MAX - 1;
    if (*y >= LV_VER_RES_MAX) *y = LV_VER_RES_MAX - 1;
    // No negative values due to uint16_t, but intermediate cal_ might be.

    // printf("Raw: (%u, %u) -> Calibrated: (%u, %u)\n", raw_x, raw_y, *x, *y); // For debugging calibration

    return true;
}
