#include "lv_port_indev.h"
#include "xpt2046.h" // Path to your XPT2046 driver
#include <stdio.h> // For printf debugging

static void xpt2046_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

static lv_indev_t * indev_touchpad; // Keep track of the input device

void lv_port_indev_init(void) {
    xpt2046_init(); // Initialize your XPT2046 driver

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = xpt2046_read_cb;

    indev_touchpad = lv_indev_drv_register(&indev_drv); // Register the driver
    if (indev_touchpad == NULL) {
        printf("ERROR: Failed to register XPT2046 input device!\n");
    } else {
        printf("LVGL Input (XPT2046) Port Initialized\n");
    }
}

static void xpt2046_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    (void)indev_drv; // Unused

    static uint16_t last_x = 0;
    static uint16_t last_y = 0;

    bool touched = xpt2046_is_touched();

    if (touched) {
        uint16_t x, y;
        if (xpt2046_get_touch_point(&x, &y)) {
            data->point.x = x;
            data->point.y = y;
            data->state = LV_INDEV_STATE_PR; // Pressed
            last_x = x; // Store last known pressed coordinates
            last_y = y;
            // printf("Touch: X=%d, Y=%d\n", x, y); // For debugging
        } else {
            // Failed to get a valid point, treat as released for this read
            data->point.x = last_x; // Use last known good coordinate
            data->point.y = last_y;
            data->state = LV_INDEV_STATE_REL; // Released
        }
    } else {
        data->point.x = last_x; // Use last known good coordinate
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL; // Released
    }
}
