#ifndef LV_PORT_H
#define LV_PORT_H
#include "lvgl.h"

#define ENC_A 6
#define ENC_B 5
#define ENC_SW 2

void lv_port_init();
void time_init();
void encoder_init();
void encoder_read(lv_indev_t *indev_drv, lv_indev_data_t *data);

#endif // LV_PORT_H