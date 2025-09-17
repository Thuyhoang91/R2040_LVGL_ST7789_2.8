#include "lv_port.h"
#include "lvgl.h"
#include "st7789.h"
#include "stdio.h"

#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "pico/time.h"
#include "hardware/timer.h"



#define buf_line  40
#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */
//lv_draw_buf_t draw_buf;
static uint8_t buf1[ST7789_WIDTH * buf_line * BYTE_PER_PIXEL];
static uint8_t buf2[ST7789_WIDTH * buf_line * BYTE_PER_PIXEL];

static lv_indev_t *encoder_indev;
lv_group_t *group;


volatile int encoder_count = 0;
volatile bool button_pressed = false;


static void encoder_callback(uint gpio, uint32_t events) {
    
    static absolute_time_t last_event = {0};
    // Debounce: Ignore events within 5ms
    if (absolute_time_diff_us(last_event, get_absolute_time()) < 5000) {
        return;
    }
    last_event = get_absolute_time();   
    
    uint8_t enc_value = (gpio_get(ENC_A) << 1) | gpio_get(ENC_B); // A=bit1, B=bit0

    // Debug: Print raw states
    //printf("GPIO: %04x, gpio_state: %02x, enc_value: %02x, gpio: %d\n",gpio_get_all(), gpio_state, enc_value, gpio);

    if(enc_value == 2){ // 10
        encoder_count++;
        printf("encoder :%d\n", encoder_count);
    } 
    if (enc_value == 1){ //01
        encoder_count--;
        printf("encoder :%d\n", encoder_count);
    } 
    if (gpio == ENC_SW){
        button_pressed = true;
        printf("button pressed\n");
    }
}

 static void encoder_read(lv_indev_t *indev_drv, lv_indev_data_t *data) {
    data->enc_diff = encoder_count;
    if(encoder_count>0) {
        data->key = LV_KEY_RIGHT;
        printf("key right\n");
    }
    if(encoder_count<0) {
        data->key = LV_KEY_LEFT;
        printf("key left\n");
    }
    //else data->key = 0;
    encoder_count = 0;
    if(button_pressed) {
        data->key = LV_KEY_ENTER;
        printf("key enter\n");
    }
    //else data->key = 0;
    data->state = button_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    button_pressed = false;
}

static void encoder_init() {
    gpio_init(ENC_A);
    gpio_set_dir(ENC_A, GPIO_IN);
    gpio_pull_up(ENC_A);

    gpio_init(ENC_B);
    gpio_set_dir(ENC_B, GPIO_IN);
    gpio_pull_up(ENC_B);

    // ENC_C is connected directly to GND

    gpio_init(ENC_SW);
    gpio_set_dir(ENC_SW, GPIO_IN);
    gpio_pull_up(ENC_SW);

    gpio_set_irq_enabled_with_callback(ENC_A, GPIO_IRQ_EDGE_FALL, true, &encoder_callback);
    gpio_set_irq_enabled(ENC_B, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ENC_SW, GPIO_IRQ_EDGE_FALL, true);
}
static bool timer_callback(struct repeating_timer *t) {
    lv_tick_inc(5); // Increment LVGL tick by 5ms
    return true;    // Continue repeating
}

static void time_init() {
    static struct repeating_timer timer;
    add_repeating_timer_us(5000, timer_callback, NULL, &timer); // 5ms interval
}


void lv_port_init() {
    // --- LVGL Init ---
    lv_init();
    time_init();
    encoder_init();

    // 1. Initialize `lv_disp_drv_t` for the display driver

    lv_disp_t *disp_drv = lv_display_create(ST7789_WIDTH, ST7789_HEIGHT);
    lv_display_set_flush_cb(disp_drv, st7789_flush_dma);


    // 2. Initialize `lv_disp_draw_buf_t` for the display buffer

    lv_display_set_buffers(disp_drv, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);


    // 3. Initialize `lv_indev_drv_t` for the input device (encoder)
    
    encoder_indev = lv_indev_create();
    lv_indev_set_type(encoder_indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(encoder_indev, encoder_read);


    // 4. Create a group and assign the input device to it

    group = lv_group_create();
    lv_indev_set_group(encoder_indev, group);
  }


