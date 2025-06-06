#include "st7789.h"
#include "pico/time.h"
#include <stdio.h> // For printf, if needed

// SPI configuration
#define SPI_BAUD_RATE (62.5 * 1000 * 1000) // 40 MHz, adjust as needed, max for ST7789 is often ~62.5 MHz

static inline void cs_select() {
    gpio_put(PIN_CS, 0);
}

static inline void cs_deselect() {
    gpio_put(PIN_CS, 1);
}

static inline void dc_command() {
    gpio_put(PIN_DC, 0);
}

static inline void dc_data() {
    gpio_put(PIN_DC, 1);
}

static inline void reset_display() {
    gpio_put(PIN_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_RST, 1);
    sleep_ms(120); // Wait for display to recover
}

void st7789_write_cmd(uint8_t cmd) {
    cs_select();
    dc_command();
    spi_write_blocking(SPI_PORT, &cmd, 1);
    cs_deselect();
}

void st7789_write_data(const uint8_t *data, size_t len) {
    cs_select();
    dc_data();
    spi_write_blocking(SPI_PORT, data, len);
    cs_deselect();
}

void st7789_write_data_byte(uint8_t data) {
    cs_select();
    dc_data();
    spi_write_blocking(SPI_PORT, &data, 1);
    cs_deselect();
}

void st7789_send_pixels(const uint16_t* pixels, size_t len) {
    cs_select();
    dc_data();
    // SPI expects uint8_t*, so cast. Also, send MSB first for 16-bit colors.
    // The ST7789 expects data in big-endian (MSB first) for 16-bit colors.
    // If your LV_COLOR_16_SWAP is 0, LVGL provides colors in RGB565 big-endian.
    // If LV_COLOR_16_SWAP is 1, LVGL provides colors in RGB565 little-endian.
    // The spi_write_blocking function sends bytes as they are.
    // We might need to swap bytes if LV_COLOR_16_SWAP is used differently than the display expects.
    // For simplicity, this example assumes LVGL provides data in the correct byte order for ST7789.
    spi_write_blocking(SPI_PORT, (const uint8_t*)pixels, len * 2); // len is number of pixels, each pixel is 2 bytes
    cs_deselect();
}

void st7789_init() {
    // Initialize GPIOs
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1); // Deselect

    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);

    gpio_init(PIN_BLK);
    gpio_set_dir(PIN_BLK, GPIO_OUT);
    // TODO: If using PWM for backlight, initialize PWM here instead
    gpio_put(PIN_BLK, 1); // Backlight ON

    // Initialize SPI
    spi_init(SPI_PORT, SPI_BAUD_RATE);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI); // If MISO is used

    // SPI format: 8 bits, CPHA=0, CPOL=0 (Mode 0) is common for ST7789
    // ST7789 datasheet says: SCK is low when CS is high (idle state). Data is latched on SCK rising edge.
    // This corresponds to SPI Mode 0 (CPOL=0, CPHA=0) or Mode 3 (CPOL=1, CPHA=1)
    // Most ST7789 modules work fine with Mode 0. Pico SDK default for spi_init is Mode 0.
    // If you have issues, try spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    // or SPI_CPOL_1, SPI_CPHA_1.

    reset_display();

    st7789_write_cmd(ST7789_SWRESET); // Software reset
    sleep_ms(150);

    st7789_write_cmd(ST7789_SLPOUT); // Sleep out
    sleep_ms(255); // ST7789 datasheet says 5ms is enough, but some displays need more

    // Memory Data Access Control (MADCTL)
    // Bit 7: MY (Row Address Order) - 0 = Top to Bottom, 1 = Bottom to Top
    // Bit 6: MX (Column Address Order) - 0 = Left to Right, 1 = Right to Left
    // Bit 5: MV (Row/Column Exchange) - 0 = Normal, 1 = Row/Column exchanged (for rotation)
    // Bit 4: ML (Vertical Refresh Order) - 0 = LCD Refresh Top to Bottom
    // Bit 3: RGB (BGR Order) - 0 = RGB, 1 = BGR
    // Bit 2: MH (Horizontal Refresh Order) - 0 = LCD Refresh Left to Right
    // Default: 0x00 (Top-Bottom, Left-Right, Normal, RGB)
    // For 240W x 320H portrait:
    //   If display native is 240x320: MADCTL = 0x00
    //   If display native is 320x240 and you want portrait: MADCTL = 0xA0 (MY=1, MV=1) or 0x60 (MX=1, MV=1)
    //   Common for 240x320 displays:
    uint8_t madctl_val = 0x00; // Default: RGB, Top-to-Bottom, Left-to-Right
    // madctl_val |= (1<<7); // MY: Row Addr Order (0=T2B, 1=B2T)
    // madctl_val |= (1<<6); // MX: Col Addr Order (0=L2R, 1=R2L)
    // madctl_val |= (1<<5); // MV: Row/Col Exchange
    // madctl_val |= (1<<3); // BGR Order (1 for BGR, 0 for RGB)

    // For landscape mode, you might want to set MV=1 (Row/Column Exchange) and adjust MY/MX accordingly.

    // 0x00: Standard portrait.
    // 0xC0: (MY | MX) Portrait, but flipped on both axes.
    // 0x60: (MX | MV) Landscape (320W x 240H).
    // 0xA0: (MY | MV) Landscape, flipped.
    // Add 0x08 (BGR bit) if colors are swapped (e.g., red looks blue).
    st7789_write_cmd(ST7789_MADCTL);
    st7789_write_data_byte(madctl_val);

    // Interface Pixel Format (COLMOD)
    // 0x55: 16 bits/pixel (RGB565)
    // 0x66: 18 bits/pixel
    st7789_write_cmd(ST7789_COLMOD);
    st7789_write_data_byte(0x55); // 16-bit color
    sleep_ms(10);

    // Optional: Inversion control if colors look weird
    // st7789_write_cmd(ST7789_INVON); // Invert display colors
    // st7789_write_cmd(ST7789_INVOFF); // Normal display colors

    // Set Porch settings if needed (usually not required for basic operation)
    /*
    st7789_write_cmd(0xB2); // Porch Setting
    st7789_write_data_byte(0x0C);
    st7789_write_data_byte(0x0C);
    st7789_write_data_byte(0x00);
    st7789_write_data_byte(0x33);
    st7789_write_data_byte(0x33);

    st7789_write_cmd(0xB7); // Gate Control
    st7789_write_data_byte(0x35);
    */

    // Set VCOM if needed
    /*
    st7789_write_cmd(ST7789_VCOMS);
    st7789_write_data_byte(0x19); // Default 0x19, try 0x2B or 0x3F if flicker
    */

    // Power Control, Gamma Settings etc. (Often defaults are fine)
    // Refer to ST7789 datasheet or example code for your specific module if needed
    // For example, some displays need:
    /*
    st7789_write_cmd(ST7789_PWCTRL1);
    uint8_t pwctrl1_data[] = {0xA4, 0xA1};
    st7789_write_data(pwctrl1_data, sizeof(pwctrl1_data));

    st7789_write_cmd(ST7789_PVGAMCTRL); // Positive Gamma Control
    uint8_t pvgam_data[] = {0xD0, 0x08, 0x0E, 0x09, 0x09, 0x05, 0x31, 0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34};
    st7789_write_data(pvgam_data, sizeof(pvgam_data));

    st7789_write_cmd(ST7789_NVGAMCTRL); // Negative Gamma Control
    uint8_t nvgam_data[] = {0xD0, 0x08, 0x0E, 0x09, 0x09, 0x05, 0x31, 0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34};
    st7789_write_data(nvgam_data, sizeof(nvgam_data));
    */

    st7789_write_cmd(ST7789_NORON);  // Normal display mode on
    sleep_ms(10);

    st7789_write_cmd(ST7789_DISPON); // Display on
    sleep_ms(100);

    // Clear screen (optional, LVGL will draw over it)
    // st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    // st7789_fill_color(0x0000, (uint32_t)ST7789_WIDTH * ST7789_HEIGHT); // Fill with black

    st7789_set_backlight(100); // Full backlight
    printf("ST7789 Initialized\n");
}

void st7789_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
    st7789_write_cmd(ST7789_CASET); // Column Address Set
    uint8_t caset_data[] = {
        (x_start >> 8) & 0xFF, x_start & 0xFF,
        (x_end >> 8) & 0xFF, x_end & 0xFF
    };
    st7789_write_data(caset_data, sizeof(caset_data));

    st7789_write_cmd(ST7789_RASET); // Row Address Set
    uint8_t raset_data[] = {
        (y_start >> 8) & 0xFF, y_start & 0xFF,
        (y_end >> 8) & 0xFF, y_end & 0xFF
    };
    st7789_write_data(raset_data, sizeof(raset_data));

    st7789_write_cmd(ST7789_RAMWR); // Memory Write
}

// For basic testing
void st7789_fill_color(uint16_t color, uint32_t len) {
    uint8_t hi = (color >> 8) & 0xFF;
    uint8_t lo = color & 0xFF;
    cs_select();
    dc_data();
    for (uint32_t i = 0; i < len; i++) {
        spi_write_blocking(SPI_PORT, &hi, 1);
        spi_write_blocking(SPI_PORT, &lo, 1);
    }
    cs_deselect();
}


void st7789_set_backlight(uint8_t brightness_percent) {
    if (brightness_percent > 100) brightness_percent = 100;
    // Simple ON/OFF for now. For PWM:
    // uint16_t duty = (brightness_percent * PWM_MAX_DUTY) / 100;
    // pwm_set_gpio_level(PIN_BLK, duty);
    if (brightness_percent > 0) {
        gpio_put(PIN_BLK, 1);
    } else {
        gpio_put(PIN_BLK, 0);
    }
}
