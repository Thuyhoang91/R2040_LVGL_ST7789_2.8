#include "st7789.h"
#include "hardware/dma.h"
#include <string.h>
#include "lv_port_disp.h"

// DMA channel
static int dma_chan;
static volatile bool dma_is_busy = false;

// Forward declaration for the ISR
static void dma_irq_handler();

// Basic command/data sending functions
static void st7789_cmd(uint8_t cmd, const uint8_t* data, size_t len) {
    gpio_put(SPI_PIN_CS, 0);
    gpio_put(LCD_PIN_DC, 0); // Command mode
    spi_write_blocking(SPI_PORT, &cmd, 1);

    if (len > 0) {
        gpio_put(LCD_PIN_DC, 1); // Data mode
        spi_write_blocking(SPI_PORT, data, len);
    }
    gpio_put(SPI_PIN_CS, 1);
}

static void st7789_data(const void* data, size_t len) {
    gpio_put(SPI_PIN_CS, 0);
    gpio_put(LCD_PIN_DC, 1); // Data mode
    spi_write_blocking(SPI_PORT, data, len);
    gpio_put(SPI_PIN_CS, 1);
}

// DMA interrupt handler
static void dma_irq_handler() {
    if (dma_channel_get_irq0_status(dma_chan)) {
        dma_channel_acknowledge_irq0(dma_chan);
        dma_is_busy = false;
        // This is the crucial part: tell LVGL the flush is ready
        lv_disp_flush_ready(lv_disp_get_default()->driver);
    }
}

void st7789_init(uint8_t madctl_val) {
    // SPI initialization
    spi_init(SPI_PORT, 62.5 * 1000 * 1000); // 20 MHz, can go higher
    gpio_set_function(SPI_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SPI_PIN_SCK, GPIO_FUNC_SPI);
    
    // Chip select is manual
    gpio_init(SPI_PIN_CS);
    gpio_set_dir(SPI_PIN_CS, GPIO_OUT);
    gpio_put(SPI_PIN_CS, 1);

    // Control pins
    gpio_init(LCD_PIN_DC);
    gpio_set_dir(LCD_PIN_DC, GPIO_OUT);
    gpio_init(LCD_PIN_RST);
    gpio_set_dir(LCD_PIN_RST, GPIO_OUT);
    gpio_init(LCD_PIN_BLK);
    gpio_set_dir(LCD_PIN_BLK, GPIO_OUT);
    gpio_put(LCD_PIN_BLK, 1); // Turn on backlight

    // Reset the display
    gpio_put(LCD_PIN_RST, 0);
    sleep_ms(100);
    gpio_put(LCD_PIN_RST, 1);
    sleep_ms(100);
    
    // --- ST7789 Initialization Sequence ---
    // (This is a standard sequence, consult your display's datasheet if needed)
    st7789_cmd(0x11, NULL, 0); // Sleep out
    sleep_ms(120);

    uint8_t color_mode[] = {0x55}; // 16-bit/pixel (RGB565)
    st7789_cmd(0x3A, color_mode, 1);


    // *** THIS IS THE KEY CHANGE FOR PORTRAIT MODE ***
    // 0x60 = Invert column order, exchange row/column
    // Your panel might require a different value (e.g., 0x00, 0x70, 0xA0).
    // Consult your display's datasheet for MADCTL options if this doesn't work.
    // uint8_t mem_access_ctrl[] = {0x60}; 
    // st7789_cmd(0x36, mem_access_ctrl, 1);
    uint8_t mem_access_ctrl[] = {madctl_val}; // Adjust for rotation/orientation
    st7789_cmd(0x36, mem_access_ctrl, 1);

    st7789_cmd(0x29, NULL, 0); // Display on
    sleep_ms(100);

    // --- DMA Initialization ---
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(SPI_PORT, true)); // DREQ for SPI TX
    channel_config_set_write_increment(&c, false); // Don't increment write address (always SPI TXDR)
    channel_config_set_read_increment(&c, true);   // Increment read address (from LVGL buffer)
    dma_channel_set_config(dma_chan, &c, false);

    // Configure DMA interrupt
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void st7789_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
    uint8_t caset_data[] = { (x_start >> 8) & 0xFF, x_start & 0xFF, (x_end >> 8) & 0xFF, x_end & 0xFF };
    st7789_cmd(0x2A, caset_data, 4); // Column Address Set

    uint8_t raset_data[] = { (y_start >> 8) & 0xFF, y_start & 0xFF, (y_end >> 8) & 0xFF, y_end & 0xFF };
    st7789_cmd(0x2B, raset_data, 4); // Row Address Set

    st7789_cmd(0x2C, NULL, 0); // Memory Write
}

void st7789_write_data_dma(const void *data, size_t len) {
    dma_is_busy = true;
    gpio_put(SPI_PIN_CS, 0);
    gpio_put(LCD_PIN_DC, 1); // Data mode

    // Configure and start DMA
    dma_channel_set_read_addr(dma_chan, data, false);
    dma_channel_set_write_addr(dma_chan, &spi_get_hw(SPI_PORT)->dr, false);
    dma_channel_set_trans_count(dma_chan, len, true); // Start transfer
}

bool st7789_dma_is_busy() {
    return dma_is_busy;
}
