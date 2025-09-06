#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "hardware/dma.h"
#include "lvgl.h"
//#include "lv_demos.h"
//#include "lv_widgets/lv_gauge.h"




#define ENCODER_PIN_A 5
#define ENCODER_PIN_B 6
#define ENCODER_BUTTON 2

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

volatile int count = 0;
//volatile uint8_t prev_state = 0;

static lv_indev_t *encoder_indev;
static lv_group_t *group;
static lv_obj_t *win;
static bool enc_pressed = false;
static lv_obj_t *scale;
static lv_obj_t *indicator; // Arc for the needle
static lv_obj_t *direction_label;
static const lv_point_precise_t needle_points[] = {{0, 0}, {0, -80}}; // Needle from center to 80px out
static lv_anim_t needle_anim; // Animation for needle rotation

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

static void encoder_read(lv_indev_t *drv, lv_indev_data_t *data) {
    // static int32_t last_enc_diff = 0;
    // int32_t enc_diff = get_encoder_diff();
    // bool enc_pressed = is_encoder_pressed();

    data->enc_diff = count; //enc_diff - last_enc_diff;
    //last_enc_diff = enc_diff;
    data->state = enc_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    // if (data->enc_diff > 0) data->key = LV_KEY_RIGHT;
    // else if (data->enc_diff < 0) data->key = LV_KEY_LEFT;
    if (lv_group_get_editing(group)) {
        // In editing mode, adjust needle angle
        if (data->enc_diff != 0 && indicator) {
            int32_t current_angle = lv_obj_get_style_transform_angle(indicator, 0) / 10; // Get current angle in degrees
            int32_t new_angle = (current_angle + data->enc_diff * 5) % 360; // 5° per step
            if (new_angle < 0) new_angle += 360; // Handle negative angles
            lv_anim_set_values(&needle_anim, current_angle * 10, new_angle * 10); // In 0.1° units
            lv_anim_start(&needle_anim);
        }
    } else {
        // In navigation mode, map rotation to focus movement
        if (data->enc_diff > 0) data->key = LV_KEY_NEXT;
        else if (data->enc_diff < 0) data->key = LV_KEY_PREV;
    }
    if (enc_pressed) data->key = LV_KEY_ENTER; // Press to interact or toggle edit mode
    count = 0; // Reset count after reading
    enc_pressed = false; // Reset button state after reading
    // No return value needed
    //return false;
}

void encoder_a_irq(uint gpio, uint32_t events) {
    static bool ccw_fall = false;
    static bool cw_fall = false;
    static absolute_time_t last_event = {0};
    
    // Debounce: Ignore events within 5ms
    if (absolute_time_diff_us(last_event, get_absolute_time()) < 1000) {
                    return;
                }
    last_event = get_absolute_time();
    
    // Read A, B, SW directly
    bool a_state = gpio_get(ENCODER_PIN_A);
    bool b_state = gpio_get(ENCODER_PIN_B);
    uint8_t enc_value = (a_state << 1) | b_state; // A=bit1, B=bit0
    //uint32_t gpio_state = (gpio_get_all() >> 2) & 0b1101;

    // Debug: Print raw states
    //printf("GPIO: %04x, gpio_state: %02x, enc_value: %02x, gpio: %d\n",gpio_get_all(), gpio_state, enc_value, gpio);

    if (gpio == ENCODER_PIN_A && events & GPIO_IRQ_EDGE_FALL) {
        if (!cw_fall && b_state) { // A falls, B high (10)
            cw_fall = true;
            //printf("CW start (A fall, B=1)\n");
        }
        if (ccw_fall && enc_value == 0b00) { // A falls after B, both low
            cw_fall = false;
            ccw_fall = false;
            count--;
            printf("Encoder: %d (CCW)\n", count);
        }
    } else if (gpio == ENCODER_PIN_B && events & GPIO_IRQ_EDGE_FALL) {
        if (!ccw_fall && a_state) { // B falls, A high (01)
            ccw_fall = true;
            //printf("CCW start (B fall, A=1)\n");
        }
        if (cw_fall && enc_value == 0b00) { // B falls after A, both low
            cw_fall = false;
            ccw_fall = false;
            count++;
            printf("Encoder: %d (CW)\n", count);
        }
    } else if (gpio == ENCODER_BUTTON && events & GPIO_IRQ_EDGE_FALL) {
        
        enc_pressed = true;
        printf("Button pressed\n");
    }
}

void encoder_init(){
    gpio_init(ENCODER_PIN_A);
    gpio_set_dir(ENCODER_PIN_A, GPIO_IN);
    gpio_pull_up(ENCODER_PIN_A);

    gpio_init(ENCODER_PIN_B);
    gpio_set_dir(ENCODER_PIN_B, GPIO_IN);
    gpio_pull_up(ENCODER_PIN_B);

    gpio_init(ENCODER_BUTTON);
    gpio_set_dir(ENCODER_BUTTON, GPIO_IN);
    gpio_pull_up(ENCODER_BUTTON);

    // Initialize prev_state
    //prev_state = (gpio_get(ENCODER_PIN_A) << 1) | gpio_get(ENCODER_PIN_B);

    // Set up interrupts for falling edge
    gpio_set_irq_enabled_with_callback(ENCODER_PIN_A, GPIO_IRQ_EDGE_FALL, true, &encoder_a_irq);
    gpio_set_irq_enabled(ENCODER_PIN_B, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ENCODER_BUTTON, GPIO_IRQ_EDGE_FALL, true);

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

#define buf_line  40
#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */
//lv_draw_buf_t draw_buf;
static uint8_t buf1[ST7789_WIDTH * buf_line * BYTE_PER_PIXEL];
static uint8_t buf2[ST7789_WIDTH * buf_line * BYTE_PER_PIXEL];


void lvgl_init() {
    // --- LVGL Init ---
    lv_init();

    // 1. Initialize `lv_disp_draw_buf_t` for the display buffer
    //lv_disp_draw_buf_init(&draw_buf, buf1, buf2, ST7789_WIDTH * buf_line);

    // 2. Initialize `lv_disp_drv_t` for the display driver
    //lv_disp_t *disp_drv;
    lv_disp_t *disp_drv = lv_display_create(ST7789_WIDTH, ST7789_HEIGHT);
    lv_display_set_flush_cb(disp_drv, st7789_flush_dma);
    lv_display_set_buffers(disp_drv, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    //lv_disp_drv_init(&disp_drv);
    //disp_drv.hor_res = ST7789_WIDTH;
    //disp_drv.ver_res = ST7789_HEIGHT;
    //disp_drv.flush_cb = st7789_flush_dma;
    //disp_drv.draw_buf = &draw_buf;
    //lv_fs_drv_register(&disp_drv);

    // 3. Initialize `lv_indev_drv_t` for the input device (encoder)
    
    lv_indev_t *indev_drv = lv_indev_create();
    lv_indev_set_type(indev_drv, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev_drv, encoder_read);
    encoder_indev = indev_drv;

    // static lv_indev_t indev_drv;
    // lv_indev_drv_init(&indev_drv);
    // indev_drv.type = LV_INDEV_TYPE_ENCODER;
    // indev_drv.read_cb = encoder_read;
    // encoder_indev = lv_indev_drv_register(&indev_drv);

    group = lv_group_create();
    lv_indev_set_group(encoder_indev, group);
}

// void setup_ui() {
//     lv_obj_t *btn = lv_btn_create(lv_scr_act());
//     lv_obj_t *label = lv_label_create(btn);
//     lv_label_set_text(label, "Click Me");
//     lv_group_add_obj(group, btn);

//     lv_obj_t *slider = lv_slider_create(lv_scr_act());
//     lv_obj_set_pos(slider, 50, 100);
//     lv_group_add_obj(group, slider);
// }

// Event callback for indicator value changes
static void indicator_event_cb(lv_event_t *e) {
    lv_obj_t *obj = lv_event_get_target(e);
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        int32_t value = lv_arc_get_value(obj); // Get angle (0-360)
        lv_obj_set_style_transform_angle(obj, value * 10, 0); // Rotate needle (angle in 0.1° units)
        // Map to compass directions
        const char *directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        int index = (value + 22) % 360 / 45; // Map to 8 directions (360/8 = 45° per direction)
        lv_label_set_text(direction_label, directions[index]);
    }
}

static void needle_anim_exec_cb(void *var, int32_t value) {
    lv_obj_set_style_transform_angle((lv_obj_t *)var, value, 0); // Set angle in 0.1° units
    // Update direction label
    const char *directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int index = (value / 10 + 22) % 360 / 45; // Map to 8 directions
    lv_label_set_text(direction_label, directions[index]);
}


// Event callback for close button
static void close_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_del(win); // Close the window
        win = NULL; // Prevent dangling pointer
    }
}
// Create compass-like gauge
void create_compass(void) {
    // Create window
    win = lv_win_create(lv_scr_act()); // Create window with default header height
    lv_win_add_title(win, "Compass");
    lv_obj_t *close_btn = lv_win_add_button(win, LV_SYMBOL_CLOSE, 10); // Close button
    lv_obj_add_event_cb(close_btn, close_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, close_btn); // Add close button to group

    // Get content area
    lv_obj_t *content = lv_win_get_content(win);
    lv_obj_set_style_pad_all(content, 10, 0); // Padding for content

    // Create scale
    scale = lv_scale_create(content);
    lv_obj_set_size(scale, 150, 150);
    lv_obj_align(scale, LV_ALIGN_CENTER, 0, 0);

    // Configure scale as a circle
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_total_tick_count(scale, 37); // 360° / 10° = 36 ticks + 1 for 0°
    lv_scale_set_major_tick_every(scale, 5); // Major tick every 5 minor ticks (~45°)
    lv_scale_set_range(scale, 0, 360);
    lv_scale_set_angle_range(scale, 360);
    lv_scale_set_rotation(scale, 270); // Start at top (0° = North)

    // Add direction labels
    static const char *labels[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW", NULL};
    lv_scale_set_text_src(scale, labels);

    // Style the scale
    static lv_style_t style_scale;
    lv_style_init(&style_scale);
    lv_style_set_line_color(&style_scale, lv_color_hex(0xFFFFFF)); // White ticks
    lv_style_set_text_color(&style_scale, lv_color_hex(0xFFFFFF)); // White labels
    lv_style_set_bg_color(&style_scale, lv_color_hex(0x333333)); // Dark background
    lv_obj_add_style(scale, &style_scale, 0);

    // Create line as needle
    indicator = lv_line_create(content);
    lv_line_set_points(indicator, needle_points, 2); // Set needle points
    lv_obj_align(indicator, LV_ALIGN_CENTER, 0, 0);

    // Style the needle
    static lv_style_t style_indicator;
    lv_style_init(&style_indicator);
    lv_style_set_line_color(&style_indicator, lv_color_hex(0x0000FF)); // Blue needle
    lv_style_set_line_width(&style_indicator, 4); // Thicker line
    lv_obj_add_style(indicator, &style_indicator, 0);
    lv_obj_set_style_transform_angle(indicator, 0, 0); // Start at 0° (North)

    // Add indicator to group for encoder control
    lv_group_add_obj(group, indicator);

    // Initialize animation
    lv_anim_init(&needle_anim);
    lv_anim_set_var(&needle_anim, indicator);
    lv_anim_set_exec_cb(&needle_anim, needle_anim_exec_cb);
    lv_anim_set_time(&needle_anim, 200); // 200ms animation duration
    lv_anim_set_path_cb(&needle_anim, lv_anim_path_ease_in_out); // Smooth easing

    // Create label to display selected direction
    direction_label = lv_label_create(content);
    lv_label_set_text(direction_label, "N");
    lv_obj_align(direction_label, LV_ALIGN_CENTER, 0, 80); // Below scale
}

static void list_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *btn = lv_event_get_target(e);
        const char *txt = lv_list_get_btn_text(lv_obj_get_parent(btn), btn);
        printf("Clicked: %s\n", txt);
    }
}

void create_symbol_list(void) {
    lv_obj_t *list = lv_list_create(lv_scr_act());
    lv_obj_set_size(list, ST7789_WIDTH, ST7789_HEIGHT);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);

    // Add items to the list and group
    lv_obj_t *btn;
    btn = lv_list_add_btn(list, LV_SYMBOL_OK, "Confirm");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "Wi-Fi");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_BLUETOOTH, "Bluetooth");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, "Settings");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_TRASH, "Delete");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_HOME, "Home");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_DOWNLOAD, "Download");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);    
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_GPS, "Location");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_VIDEO, "Video");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_KEYBOARD, "keyboard");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_EDIT, "Edit");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_SAVE, "Save");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
    btn = lv_list_add_btn(list, LV_SYMBOL_CLOSE, "Close");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(group, btn);
}
int main()
{
    stdio_init_all();
    //encoder_init();
    st7789_init();
    encoder_init();
    dma_init();
    time_init();
    lvgl_init();    
    //lv_demo_widgets(); // Start a demo to showcase LVGL features
    //setup_ui();
    //create_symbol_list();
    create_compass();

    while (true) {
        lv_task_handler(); // Handle LVGL tasks
        tight_loop_contents(); // Idle loop
    }
    return 0;
}
