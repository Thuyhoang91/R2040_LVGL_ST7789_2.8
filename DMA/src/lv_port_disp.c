// File: lv_port_disp.c

#include "lvgl.h"
#include "lv_port_disp.h"
#include "st7789.h"

// LVGL draw buffers
// For performance, using two buffers is recommended.
// The size can be smaller than the screen, e.g., 1/10th of the screen size.
#define BUF_LINES 200
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf_1[LCD_WIDTH * BUF_LINES];
static lv_color_t buf_2[LCD_WIDTH * BUF_LINES];

// The LVGL flush callback
static void disp_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    // Wait until the previous DMA transfer is complete
    while (st7789_dma_is_busy()) {}

    // Set the display window to the area being updated
    st7789_set_window(area->x1, area->y1, area->x2, area->y2);

    // Start the DMA transfer
    size_t len = lv_area_get_size(area) * sizeof(lv_color_t);
    st7789_write_data_dma(color_p, len);

    // Note: lv_disp_flush_ready() is called from the DMA ISR in st7789_drive.c
}

// Initialize the display port
void lv_port_disp_init(void) {
    uint8_t madctl_val = 0;
     // --- Translate rotation setting to MADCTL register value ---
    // These values control how the display memory is scanned out.
    // The bits are: MY (row addr order), MX (col addr order), MV (row/col exchange)
    // IMPORTANT: These values might need to be tweaked for your specific display module,
    // as some have the panel mounted differently.
    // Define the rotation value if not already defined elsewhere
    
    // if (LV_PORT_DISP_ROTATION_VALUE == 1 || LV_PORT_DISP_ROTATION_VALUE == 3) {
    //     // Landscape modes (90 or 270 degrees) require swapping width and height
    //     uint8_t temp_LCD;
    //     temp_LCD = LCD_HEIGHT;
    //     LCD_HEIGHT = LCD_WIDTH;
    //     LCD_WIDTH = temp_LCD;
    // }

    switch (LV_PORT_DISP_ROTATION_VALUE) {
        case LV_PORT_ROT_90:
            // Landscape, USB port on the right           
            madctl_val = 0xA0; // MY=1, MX=0, MV=1
            break;
        case LV_PORT_ROT_180:
            // Inverted Portrait, USB port at the top
            madctl_val = 0xC0; // MY=1, MX=1, MV=0
            break;
        case LV_PORT_ROT_270:
            // Inverted Landscape, USB port on the left
            madctl_val = 0x60; // MY=0, MX=1, MV=1
            break;
        default: // LV_PORT_ROT_NONE
            // Portrait, USB port at the bottom
            madctl_val = 0x00; // MY=0, MX=0, MV=0
            break;
    }
    // 1. Initialize the low-level ST7789 driver
    st7789_init(madctl_val);

    // 2. Initialize LVGL draw buffers
    lv_disp_draw_buf_init(&draw_buf, buf_1, buf_2, LCD_WIDTH * BUF_LINES);

    // 3. Initialize LVGL display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    // Assign the driver to the display
    if(LV_PORT_DISP_ROTATION_VALUE == 1 || LV_PORT_DISP_ROTATION_VALUE == 3) {
        // Landscape modes require swapping width and height
        disp_drv.hor_res = LCD_HEIGHT;
        disp_drv.ver_res = LCD_WIDTH;
    } else {
        // Portrait mode
        disp_drv.hor_res = LCD_WIDTH;
        disp_drv.ver_res = LCD_HEIGHT;
    }
    // disp_drv.hor_res = LCD_WIDTH;
    // disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    
    // Use partial updates. Set to 1 if you want to force full screen refreshes.
    disp_drv.full_refresh = 0; 
    // IMPORTANT: Tell LVGL that the driver is NOT handling rotation.
    // The rotation is handled by the hardware, so LVGL's coordinates are already correct.
    //disp_drv.sw_rotate = 2;
    
    lv_disp_drv_register(&disp_drv);
}
