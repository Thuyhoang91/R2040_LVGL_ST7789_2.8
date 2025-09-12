#include "st7789.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

int dma_chan;
volatile bool dma_complete = true;
static lv_disp_t *disp_drv_ptr;

static void dma_irq_handler() {
    if (dma_hw->ints0 & (1u << dma_chan)) {
        dma_hw->ints0 = 1u << dma_chan;
        dma_complete = true;
        gpio_put(PIN_CS, 1);
        lv_disp_flush_ready(disp_drv_ptr);
    }
}

static void dma_init() {
    dma_chan = dma_claim_unused_channel(true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_set_irq0_enabled(dma_chan, true);
}

void st7789_init() {
    dma_init();
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 0);
    sleep_ms(50);
    gpio_put(PIN_RST, 1);
    sleep_ms(50);

    spi_init(SPI_PORT, SPI_BAUD);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

    uint8_t init_seq[] = {
        0x01, 0,       // Software reset
        0x11, 0,       // Sleep out
        0x36, 1, 0x00, // Memory access control
        0x3A, 1, 0x05, // Pixel format (16-bit)
        0x20, 0,       // Display inversion on
        0x29, 0,       // Display on
        0xFF           // End
    };
    uint8_t *cmd = init_seq;
    while (*cmd != 0xFF) {
        gpio_put(PIN_CS, 0);
        gpio_put(PIN_DC, 0);
        spi_write_blocking(SPI_PORT, cmd, 1);
        cmd++;
        if (*cmd > 0) {
            gpio_put(PIN_DC, 1);
            spi_write_blocking(SPI_PORT, cmd + 1, *cmd);
            cmd += *cmd + 1;
        } else {
            cmd++;
        }
        gpio_put(PIN_CS, 1);
        sleep_ms(10);
    }
}

static void st7789_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint8_t data[4];
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    spi_write_blocking(SPI_PORT, (uint8_t[]){0x2A}, 1);
    gpio_put(PIN_DC, 1);
    data[0] = x >> 8; data[1] = x & 0xFF; data[2] = (x + w - 1) >> 8; data[3] = (x + w - 1) & 0xFF;
    spi_write_blocking(SPI_PORT, data, 4);

    gpio_put(PIN_DC, 0);
    spi_write_blocking(SPI_PORT, (uint8_t[]){0x2B}, 1);
    gpio_put(PIN_DC, 1);
    data[0] = y >> 8; data[1] = y & 0xFF; data[2] = (y + h - 1) >> 8; data[3] = (y + h - 1) & 0xFF;
    spi_write_blocking(SPI_PORT, data, 4);

    gpio_put(PIN_DC, 0);
    spi_write_blocking(SPI_PORT, (uint8_t[]){0x2C}, 1);
    gpio_put(PIN_DC, 1);
}

void st7789_flush_dma(lv_disp_t *disp_drv, const lv_area_t *area, uint8_t *color_p) {
    if (!dma_complete) return;
    dma_complete = false;
    disp_drv_ptr = disp_drv;

    st7789_set_window(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);

    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_dreq(&cfg, spi_get_dreq(SPI_PORT, true));
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_read_increment(&cfg, true);

    dma_channel_configure(dma_chan, &cfg, &spi_get_hw(SPI_PORT)->dr, color_p,
                         (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1) * 2, true);
}