#include "lvgl.h"

extern  lv_group_t *group;
void setup_ui() {
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Click Me");
    lv_group_add_obj(group, btn);

    lv_obj_t *slider = lv_slider_create(lv_scr_act());
    lv_obj_set_pos(slider, 50, 100);
    lv_group_add_obj(group, slider);
}