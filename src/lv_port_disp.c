#include "lv_port_disp.h"
#include "st7789.h" // Path to your ST7789 driver
#include "pico/stdlib.h" // For printf
#include "stdio.h" // For printf, if needed


// Define the display width and height from LVGL's perspective
// These should match LV_HOR_RES_MAX and LV_VER_RES_MAX in lv_conf.h
#define DISP_HOR_RES    ST7789_WIDTH
#define DISP_VER_RES    ST7789_HEIGHT

// Display buffer size:
// Option 1: Full frame buffer (requires DISP_HOR_RES * DISP_VER_RES * LV_COLOR_DEPTH/8 bytes of RAM)
// Example: 240 * 320 * 2 bytes = 153600 bytes (Too much for RP2040's 264KB RAM if other things are used)
//
// Option 2: Partial buffer (e.g., 1/10th of the screen)
// This is more memory-efficient.
#define DISP_BUF_SIZE (DISP_HOR_RES * 32) // Buffer for 32 lines

static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[DISP_BUF_SIZE];
#if DISP_BUF_SIZE < (DISP_HOR_RES * DISP_VER_RES)
static lv_color_t buf_2[DISP_BUF_SIZE]; // Use two buffers if not full frame
#else
// No second buffer needed for full frame
#endif

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

void lv_port_disp_init(void) {
    st7789_init(); // Initialize your ST7789 driver

    lv_disp_draw_buf_init(&disp_buf, buf_1, 
#if DISP_BUF_SIZE < (DISP_HOR_RES * DISP_VER_RES)
                        buf_2, 
#else
                        NULL,
#endif
                        DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = DISP_HOR_RES;
    disp_drv.ver_res = DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &disp_buf;
    // disp_drv.full_refresh = 1; // Set to 1 if you always want to refresh the whole screen
                                  // Set to 0 if you want LVGL to only update changed areas (more efficient)
    // disp_drv.rounder_cb = disp_rounder; // Optional: if your hardware requires specific alignments
    // disp_drv.set_px_cb = disp_set_px; // Optional: for direct pixel setting (slower)

    lv_disp_drv_register(&disp_drv);
    printf("LVGL Display Port Initialized\n");
}

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;

    // The ST7789 driver expects absolute coordinates
    st7789_set_window(x1, y1, x2, y2);

    size_t Bpp = 2; // Bytes per pixel for RGB565
    size_t len = (x2 - x1 + 1) * (y2 - y1 + 1); // Number of pixels

    // Send pixel data to ST7789
    // color_p is already in the correct format (RGB565 with potential byte swap from lv_conf.h)
    st7789_send_pixels((const uint16_t *)color_p, len);

    // IMPORTANT: Inform LVGL that flushing is done
    lv_disp_flush_ready(disp_drv);
}

/* Optional rounder function if your hardware has specific alignment requirements for transfers */
// static void disp_rounder(lv_disp_drv_t * disp_drv, lv_area_t * area)
// {
//   // Example: round to even coordinates
//   // area->x1 = area->x1 & ~1;
//   // area->y1 = area->y1 & ~1;
//   // area->x2 = (area->x2 & ~1) | 1;
//   // area->y2 = (area->y2 & ~1) | 1;
// }

/* Optional set_px function for direct pixel drawing (slower than block transfers) */
// static void disp_set_px(lv_disp_drv_t * disp_drv, uint8_t * buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
//                         lv_color_t color, lv_opa_t opa)
// {
//     // This function is called by LVGL to draw individual pixels.
//     // For performance, it's better to use the flush_cb with block transfers.
//     // If you must implement this, you'd convert (x,y) and color to your display's format
//     // and write the pixel.
//     // For ST7789, you'd typically set a 1x1 window and send the color data.
//
//     // Example (pseudo-code, very inefficient for ST7789):
//     // st7789_set_window(x, y, x, y);
//     // uint16_t c = color.full; // Assuming LV_COLOR_DEPTH is 16
//     // if (LV_COLOR_16_SWAP) { c = (c >> 8) | (c << 8); }
//     // st7789_send_pixels(&c, 1);
// }
