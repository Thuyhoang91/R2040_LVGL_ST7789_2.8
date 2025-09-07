#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "hardware/dma.h"
#include "lvgl.h"
// Encoder and button pins
#define ENC_A_PIN 5
#define ENC_B_PIN 6
#define ENC_BTN_PIN 2


// #define ENCODER_PIN_A 5
// #define ENCODER_PIN_B 6
// #define ENCODER_BUTTON 2

//////////////////////////
// Pin definitions
//#define ENC_A 5
//#define ENC_B 6
//#define ENC_SW 2
#define PIN_CS 22
#define PIN_RST 21
#define PIN_DC 20
#define PIN_MOSI 19
#define PIN_SCK 18
#define SPI_PORT spi0
#define SPI_BAUD 62.5*1000*1000 // 62.5 MHz
#define ST7789_WIDTH 240
#define ST7789_HEIGHT 320

//static lv_indev_t *encoder_indev;
static lv_group_t *group;

//dma
int dma_chan;
volatile bool dma_complete = true;
static lv_disp_t *disp_drv_ptr;

void dma_irq_handler() {
    if (dma_hw->ints0 & (1u << dma_chan)) {
        dma_hw->ints0 = 1u << dma_chan;
        dma_complete = true;
        gpio_put(PIN_CS, 1);
        lv_disp_flush_ready(disp_drv_ptr);
    }
}

void dma_init() {
    dma_chan = dma_claim_unused_channel(true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_set_irq0_enabled(dma_chan, true);
}

void st7789_init() {
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

void st7789_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
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

bool timer_callback(struct repeating_timer *t) {
    lv_tick_inc(5); // Increment LVGL tick by 5ms
    return true;    // Continue repeating
}

void time_init() {
    static struct repeating_timer timer;
    add_repeating_timer_us(5, timer_callback, NULL, &timer); // 5ms interval
}

// Encoder state
static volatile int32_t encoder_count = 0;
static volatile bool button_pressed = false;
static uint32_t last_button_time = 0;
static bool last_button_state = false;
#define DEBOUNCE_MS 20  // Button debounce time

// LVGL input device
static lv_indev_t *encoder_indev;

// Encoder interrupt handler
static void encoder_isr(uint gpio, uint32_t events) {
    static uint8_t last_state = 0;
    uint8_t current_state = (gpio_get(ENC_A_PIN) << 1) | gpio_get(ENC_B_PIN);
    if(gpio == ENC_BTN_PIN){
        button_pressed = true;
        //printf("btn\n");  // Pressed on falling edge

        return;
    }
    printf("current_state: %d, last_state: %d\n", current_state, last_state);
    // Simple quadrature decode
    if (current_state != last_state) {
        // if ((last_state == 0b00 && current_state == 0b01) ||
        //     (last_state == 0b01 && current_state == 0b11) ||
        //     (last_state == 0b11 && current_state == 0b10) ||
        //     (last_state == 0b10 && current_state == 0b00)) {
        if(current_state == 1)
            encoder_count++;  // Clockwise

        // } else if ((last_state == 0b00 && current_state == 0b10) ||
        //            (last_state == 0b10 && current_state == 0b11) ||
        //            (last_state == 0b11 && current_state == 0b01) ||
        //            (last_state == 0b01 && current_state == 0b00)) {
        else if(current_state == 2)
            encoder_count--;  // Counterclockwise

        
        last_state = current_state;
    }
}

// // Button interrupt handler
// static void button_isr(uint gpio, uint32_t events) {
//     printf("btn\n");

// }

// LVGL read callback
static void encoder_read(lv_indev_t *indev, lv_indev_data_t *data) {
    static int32_t last_count = 0;
    int32_t diff = encoder_count - last_count;  // Calculate rotation delta
    last_count = encoder_count;

    // Map encoder rotation to keys
    if (diff > 0) {
        data->key = LV_KEY_RIGHT;  // Clockwise: Right key
        //data->state = LV_INDEV_STATE_PRESSED;
        printf("cw\n");
    } else if (diff < 0) {
        data->key = LV_KEY_LEFT;   // Counterclockwise: Left key
        //data->state = LV_INDEV_STATE_PRESSED;
        printf("ccw\n");
    } else {
        // Handle button press
        //bool raw_button = button_pressed;
        static absolute_time_t last_event = {0};

        if ( button_pressed ) {
            // 20ms debounce
            if (absolute_time_diff_us(last_event, get_absolute_time()) < 20000) {
                    return;
                }
            last_event = get_absolute_time();
            data->key = LV_KEY_ENTER;  // Button: Enter key
            data->state = button_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
            printf("btn\n");  // Pressed on falling edge
            button_pressed = false;  // Reset button state
        }
         else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
    }

    // Set enc_diff for rotary-sensitive widgets (e.g., sliders)
    data->enc_diff = diff;
}

// Button event callback
static void btn_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    static bool pressed = false;
    pressed = !pressed;
    lv_label_set_text(label, pressed ? "Pressed!" : "Click me!");
}

#define buf_line  40
#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */
//lv_draw_buf_t draw_buf;
static uint8_t buf1[ST7789_WIDTH * buf_line * BYTE_PER_PIXEL];
static uint8_t buf2[ST7789_WIDTH * buf_line * BYTE_PER_PIXEL];
// Initialize encoder and LVGL input
void encoder_init(void) {
    // Initialize GPIO pins
    gpio_init(ENC_A_PIN);
    gpio_init(ENC_B_PIN);
    gpio_set_dir(ENC_A_PIN, GPIO_IN);
    gpio_set_dir(ENC_B_PIN, GPIO_IN);
    gpio_pull_up(ENC_A_PIN);  // Enable pull-ups (if needed)
    gpio_pull_up(ENC_B_PIN);

    // Initialize button pin
    gpio_init(ENC_BTN_PIN);
    gpio_set_dir(ENC_BTN_PIN, GPIO_IN);
    gpio_pull_up(ENC_BTN_PIN);  // Internal pull-up

    // Set up interrupts
    gpio_set_irq_enabled_with_callback(ENC_A_PIN,GPIO_IRQ_EDGE_FALL, true, &encoder_isr);
    gpio_set_irq_enabled(ENC_B_PIN,GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ENC_BTN_PIN, GPIO_IRQ_EDGE_FALL, true);

    // Initialize LVGL
    lv_init();

    // 2. Initialize `lv_disp_drv_t` for the display driver
    lv_disp_t *disp_drv = lv_display_create(ST7789_WIDTH, ST7789_HEIGHT);
    lv_display_set_flush_cb(disp_drv, st7789_flush_dma);
    lv_display_set_buffers(disp_drv, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Initialize LVGL input device
    lv_indev_t *indev_drv = lv_indev_create();
    lv_indev_set_type(indev_drv, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev_drv, encoder_read);
    //lv_style_t *encoder_indev;
    encoder_indev = indev_drv;   

    // Create a group for navigation
    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_set_group(encoder_indev, group);    
}

void ui(){
    // Example: Add a slider to the group
    lv_obj_t *slider = lv_slider_create(lv_screen_active());
    lv_slider_set_range(slider, 0, 100);
    lv_obj_center(slider);
    // Apply style with reduced sensitivity
    static lv_style_t slider_style;
    lv_style_init(&slider_style);
    lv_style_set_rotary_sensitivity(&slider_style, 64);  // 0.25x sensitivity
    lv_obj_add_style(slider, &slider_style, 0);
    lv_group_add_obj(group, slider);

    // Example: Create a button with default sensitivity
    lv_obj_t *btn = lv_button_create(lv_screen_active());
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Click me!");
    lv_obj_center(btn);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    // Optional: Different sensitivity for button
    static lv_style_t btn_style;
    lv_style_init(&btn_style);
    lv_style_set_rotary_sensitivity(&btn_style, 128);  // 0.5x sensitivity
    lv_obj_add_style(btn, &btn_style, 0);
    lv_group_add_obj(group, btn);
}

// Main function
int main(void) {
    stdio_init_all();  // Initialize UART/USB for debugging
    // Initialize your display driver here (e.g., SPI TFT)
    // ...
    st7789_init();  // Initialize ST7789 display
    dma_init();     // Initialize DMA for display
    time_init();    // Initialize LVGL tick timer

    encoder_init();    // Initialize encoder
    ui();              // Create UI elements

    // LVGL timer for input polling
    while (1) {
        lv_task_handler();  // Process LVGL events
        lv_indev_read(encoder_indev);  // Read encoder input
        sleep_ms(5);  // Poll every 5ms
        //tight_loop_contents(); // Idle loop

    }
    return 0;
}