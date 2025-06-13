#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
// #include "hardware/irq.h"
// #include "hardware/timer.h"
#include "lvgl.h"
#include "lv_demos.h"

// =======================================================================
// Hardware Configuration
// =======================================================================

// Display: ST7789 on spi0
#define DISPLAY_SPI_PORT spi0
#define PIN_DISP_MOSI 19
#define PIN_DISP_SCK  18
#define PIN_DISP_CS   17
#define PIN_DISP_DC   20
#define PIN_DISP_RST  21
#define PIN_DISP_BLK  22
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320

// Touchscreen: XPT2046 on spi1
#define TOUCH_SPI_PORT spi1
#define PIN_TOUCH_MOSI 11 // T_DIN
#define PIN_TOUCH_MISO 12 // T_DO
#define PIN_TOUCH_SCK  10 // T_CLK
#define PIN_TOUCH_CS   13 // T_CS
#define PIN_TOUCH_IRQ  14  // T_IRQ

// --- Touchscreen Calibration ---
// YOU MUST CALIBRATE THESE VALUES FOR YOUR SPECIFIC SCREEN
#define XPT2046_MIN_X 300
#define XPT2046_MAX_X 3800
#define XPT2046_MIN_Y 250
#define XPT2046_MAX_Y 3750

// =======================================================================
// Global State for DMA and LVGL
// =======================================================================
static int dma_chan;
static volatile bool dma_transfer_complete = true;
static lv_display_t *disp_for_dma = NULL; // Store the display handle for the ISR

// LVGL display buffers
#define LVGL_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 10)
static lv_color_t buf1[LVGL_BUFFER_SIZE];
static lv_color_t buf2[LVGL_BUFFER_SIZE];

// ST7789 COMMANDS
#define ST7789_CMD_NOP        0x00
#define ST7789_CMD_SWRESET    0x01
#define ST7789_CMD_SLPIN      0x10
#define ST7789_CMD_SLPOUT     0x11
#define ST7789_CMD_INVOFF     0x20
#define ST7789_CMD_INVON      0x21
#define ST7789_CMD_DISPOFF    0x28
#define ST7789_CMD_DISPON     0x29
#define ST7789_CMD_CASET      0x2A
#define ST7789_CMD_RASET      0x2B
#define ST7789_CMD_RAMWR      0x2C
#define ST7789_CMD_COLMOD     0x3A
#define ST7789_CMD_MADCTL     0x36

// XPT2046 COMMANDS
#define XPT2046_CMD_READ_X    0xD0  // Read X position
#define XPT2046_CMD_READ_Y    0x90  // Read Y position
// XPT2046 TOUCHSCREEN CONFIGURATION
#define XPT2046_CMD_READ_Z1   0xB0  // Read Z1 (pressure)
#define XPT2046_CMD_READ_Z2   0xC0  // Read Z2 (ground)

// Orientation and color order settings
typedef enum {
    ORIENT_0 = 0x00,   // Portrait
    ORIENT_90 = 0x60,  // Landscape
    ORIENT_180 = 0xC0, // Portrait (upside down)
    ORIENT_270 = 0xA0  // Landscape (reversed)
} orientation_t;
 
#define orientation_valuE    ORIENT_0

typedef enum {
    COLOR_ORDER_RGB = 0x00, // RGB color order
    COLOR_ORDER_BGR = 0x08  // BGR color order
} color_order_t;

// =======================================================================
// ST7789 Low-Level Driver
// =======================================================================
// SPI functions for display
void spi_write_byte_disp(uint8_t data) {
    spi_write_blocking(DISPLAY_SPI_PORT, &data, 1);
}

void st7789_write_cmd(uint8_t cmd) {
    gpio_put(PIN_DISP_CS, 0);
    gpio_put(PIN_DISP_DC, 0); // Command mode
    spi_write_byte_disp(cmd);
    gpio_put(PIN_DISP_CS, 1);
}

void st7789_write_data(uint8_t data) {
    gpio_put(PIN_DISP_CS, 0);
    gpio_put(PIN_DISP_DC, 1); // Data mode
    spi_write_byte_disp(data);
    gpio_put(PIN_DISP_CS, 1);
}

// DMA interrupt handler for display
void dma_handler(void) {
    dma_hw->ints0 = 1u << dma_chan; // Clear interrupt
    dma_transfer_complete = true;
    gpio_put(PIN_DISP_CS, 1); // Deassert CS
    if (disp_for_dma) {
        lv_display_flush_ready(disp_for_dma); // Signal LVGL flush complete
    }
}

// Initialize the display
void st7789_init(orientation_t orientation, color_order_t color_order, uint8_t fre) {
    // initialize SPI0 for display
    spi_init(DISPLAY_SPI_PORT, fre * 1000 * 1000); // 62.5 MHz
    gpio_set_function(PIN_DISP_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_DISP_MOSI, GPIO_FUNC_SPI);
    
    // Initialize display control pins
    gpio_init(PIN_DISP_CS);
    gpio_init(PIN_DISP_DC);
    gpio_init(PIN_DISP_RST);
    gpio_set_dir(PIN_DISP_CS, GPIO_OUT);
    gpio_set_dir(PIN_DISP_DC, GPIO_OUT);
    gpio_set_dir(PIN_DISP_RST, GPIO_OUT);
    gpio_put(PIN_DISP_CS, 1);
    gpio_put(PIN_DISP_DC, 1);

    // Reset display
    gpio_put(PIN_DISP_RST, 0);
    sleep_ms(100);
    gpio_put(PIN_DISP_RST, 1);
    sleep_ms(100);

    // Initialize ST7789
    st7789_write_cmd(ST7789_CMD_SWRESET);
    sleep_ms(150);
    st7789_write_cmd(ST7789_CMD_SLPOUT);
    sleep_ms(10);

    st7789_write_cmd(ST7789_CMD_COLMOD);
    st7789_write_data(0x55); // 16-bit color (RGB565)
    sleep_ms(10);

    st7789_write_cmd(ST7789_CMD_MADCTL);
    st7789_write_data(orientation | color_order); // Combine orientation and color order
    sleep_ms(10);

    st7789_write_cmd(ST7789_CMD_DISPON);
    sleep_ms(100);

    // Initialize DMA for display
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_dreq(&cfg, spi_get_dreq(DISPLAY_SPI_PORT, true));
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_read_increment(&cfg, true);
    dma_channel_configure(
        dma_chan,
        &cfg,
        &spi_get_hw(DISPLAY_SPI_PORT)->dr, // Write to SPI data register
        NULL, // Set read address later
        0,    // Set transfer count later
        false // Don't start yet
    );
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    printf("ST7789 display initialized.\n");   
       
}

// Set address window
void st7789_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    st7789_write_cmd(ST7789_CMD_CASET);
    st7789_write_data(x0 >> 8);
    st7789_write_data(x0 & 0xFF);
    st7789_write_data(x1 >> 8);
    st7789_write_data(x1 & 0xFF);

    st7789_write_cmd(ST7789_CMD_RASET);
    st7789_write_data(y0 >> 8);
    st7789_write_data(y0 & 0xFF);
    st7789_write_data(y1 >> 8);
    st7789_write_data(y1 & 0xFF);

    st7789_write_cmd(ST7789_CMD_RAMWR);
}

// LVGL flush callback for display
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    if (dma_transfer_complete) {
        uint32_t w = (area->x2 - area->x1 + 1);
        uint32_t h = (area->y2 - area->y1 + 1);
        uint32_t len = w * h * 2; // 2 bytes per pixel (RGB565)

        st7789_set_addr_window(area->x1, area->y1, area->x2, area->y2);
        gpio_put(PIN_DISP_DC, 1);
        gpio_put(PIN_DISP_CS, 0);

        dma_transfer_complete = false;
        disp_for_dma = disp;
        dma_channel_set_read_addr(dma_chan, px_map, false);
        dma_channel_set_trans_count(dma_chan, len, true); // Start DMA
    }
}



// =======================================================================
// XPT2046 Low-Level Driver
// =======================================================================

static void xpt2046_get_raw(uint8_t cmd, uint8_t *buf, size_t len) {
    gpio_put(PIN_TOUCH_CS, 0);
    spi_write_blocking(TOUCH_SPI_PORT, &cmd, 1);
    spi_read_blocking(TOUCH_SPI_PORT, 0x00, buf, len);
    gpio_put(PIN_TOUCH_CS, 1);
}

bool xpt2046_is_touched() {
    return (gpio_get(PIN_TOUCH_IRQ) == 0);
}

void xpt2046_get_raw_point(uint16_t *x, uint16_t *y) {
    uint8_t buf[2];
    xpt2046_get_raw(0xD0, buf, 2); *x = (buf[0] << 8 | buf[1]) >> 3;
    xpt2046_get_raw(0x90, buf, 2); *y = (buf[0] << 8 | buf[1]) >> 3;
}

long map_value(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void my_touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
    static uint16_t last_x = 0, last_y = 0;
    if (xpt2046_is_touched()) {
        uint16_t raw_x, raw_y;
        xpt2046_get_raw_point(&raw_x, &raw_y);
        //invert x
        switch (orientation_valuE)
        {
        case ORIENT_90:
            last_x = map_value(raw_y,XPT2046_MIN_Y, XPT2046_MAX_Y, 0,DISPLAY_HEIGHT -1);        
            last_y = map_value(raw_x, XPT2046_MIN_X, XPT2046_MAX_X, 0, DISPLAY_WIDTH - 1);
            break;

        case ORIENT_180:
            last_x = map_value(raw_x,XPT2046_MIN_X, XPT2046_MAX_X, 0,DISPLAY_WIDTH -1);        
            last_y = map_value(raw_y, XPT2046_MAX_Y, XPT2046_MIN_Y, 0, DISPLAY_HEIGHT - 1);
            break;

        case ORIENT_270:
            last_x = map_value(raw_y,XPT2046_MIN_Y, XPT2046_MAX_Y, 0,DISPLAY_HEIGHT -1);        
            last_y = map_value(raw_x, XPT2046_MIN_X, XPT2046_MAX_X, 0, DISPLAY_WIDTH - 1);
            break;
        
        default:
            last_x = map_value(raw_x,XPT2046_MAX_X, XPT2046_MIN_X, 0,DISPLAY_WIDTH -1);        
            last_y = map_value(raw_y, XPT2046_MIN_Y, XPT2046_MAX_Y, 0, DISPLAY_HEIGHT - 1);
            break;
        } 
        // last_x = map_value(raw_x,XPT2046_MAX_X, XPT2046_MIN_X, 0,DISPLAY_WIDTH -1);        
        // last_y = map_value(raw_y, XPT2046_MIN_Y, XPT2046_MAX_Y, 0, DISPLAY_HEIGHT - 1);
        data->state = LV_INDEV_STATE_PRESSED;
        // To calibrate, uncomment the line below:
         printf("Raw: %d, %d -> Mapped: %d, %d\n", raw_x, raw_y, last_x, last_y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    data->point.x = last_x;
    data->point.y = last_y;
}

bool repeating_timer_callback(struct repeating_timer *t) {
    lv_tick_inc(5);
    return true;
}

// =======================================================================
// UI Application Code
// =======================================================================
static lv_obj_t * main_label;

static void button_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_label_set_text(main_label, "DMA Transfer Complete!");
        printf("Button was clicked!\n");
    }
}

void create_demo_ui() {
    main_label = lv_label_create(lv_screen_active());
    lv_label_set_text(main_label, "Hello Pico! Click the button.");
    lv_obj_align(main_label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t * btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn, 140, 50);
    lv_obj_set_pos(btn,20,80);
    lv_obj_add_event_cb(btn, button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me");
    lv_obj_center(btn_label);
}


// initiale touch
void touch_init(uint8_t fre){
    spi_init(TOUCH_SPI_PORT, fre * 1000 * 1000);
    gpio_set_function(PIN_TOUCH_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_TOUCH_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_TOUCH_MISO, GPIO_FUNC_SPI);
    gpio_init(PIN_TOUCH_CS); gpio_set_dir(PIN_TOUCH_CS, GPIO_OUT); gpio_put(PIN_TOUCH_CS, 1);
    gpio_init(PIN_TOUCH_IRQ); gpio_set_dir(PIN_TOUCH_IRQ, GPIO_IN); gpio_pull_up(PIN_TOUCH_IRQ);
    printf("Display and Touch initialized.\n");
}

void lv_port_init(){
    //initial lvgl
    lv_init();
    //initial display
    st7789_init(ORIENT_0, COLOR_ORDER_RGB, 62.5);
    // initial touch
    touch_init(0.1);

    //Set up display with triple buffers
    lv_display_t *disp;
    if(ORIENT_0 || ORIENT_180){
        disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }
    else {
        disp = lv_display_create(DISPLAY_HEIGHT, DISPLAY_WIDTH);
    }

    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, buf1, buf2, LVGL_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // setup device touch
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touch_read);
    lv_indev_set_display(indev, disp);
}
// =======================================================================
// Main Entry Point
// =======================================================================
int main() {
    stdio_init_all();
    printf("--- LVGL v10 Pico DMA Demo ---\n");
    // initiale display
    //initiale touch

    lv_port_init();

    struct repeating_timer timer;
    add_repeating_timer_ms(-5, repeating_timer_callback, NULL, &timer);
    printf("LVGL core initialized.\n");

    // --- Application Start ---
    //create_demo_ui();

    //add example demos
    lv_demo_widgets();



    printf("UI created. Entering main loop.\n");

    while (1) {
        lv_timer_handler();
        sleep_ms(5);
    }
    return 0;
}
