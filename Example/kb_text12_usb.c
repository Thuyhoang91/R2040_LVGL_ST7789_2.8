#include "lvgl.h"
#include "stdio.h"
#include <string.h>

#if LV_USE_KEYBOARD && LV_BUILD_EXAMPLES

// LVGL objects
lv_obj_t *ta1, *ta;
static lv_obj_t *kb;
extern lv_group_t *group;

static void kb_num_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        const char *txt = lv_buttonmatrix_get_button_text(kb, lv_buttonmatrix_get_selected_button(kb));
        if (strcmp(txt, LV_SYMBOL_UPLOAD) == 0) {
            char *content = lv_textarea_get_text(ta);
            //usb_tx(content);
            //usb_tx("\r\n");  // Newline for readability
            //lv_textarea_set_text(ta, "");  // Clear after transmit
            printf("Transmitted: %s\n", content);  // Local debug
            lv_textarea_set_text(ta, "");  // Clear after transmit

        }
    }
}

void kb_ta_map(void)
{
   static const char * kb_map[] = {"G0", "G1", "X1", "X10", "Y1", "Y10", "Z1", "Z10", LV_SYMBOL_BACKSPACE, "\n",
                                    "Q", "S", "D", "F", "G", "J", "K", "L", "M",  LV_SYMBOL_NEW_LINE, "\n",
                                    "W", "X", "C", "V", "B", "N", ",", ".", ":", "!", "?", "\n",
                                    LV_SYMBOL_CLOSE, " ",  " ", " ", LV_SYMBOL_UPLOAD, NULL
                                   };

    /*Set the relative width of the buttons and other controls*/
    static const lv_buttonmatrix_ctrl_t kb_ctrl[] = {LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_6,
                                                     LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_6,
                                                     LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4, LV_BUTTONMATRIX_CTRL_WIDTH_4,
                                                     LV_BUTTONMATRIX_CTRL_WIDTH_2, (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_HIDDEN | LV_BUTTONMATRIX_CTRL_WIDTH_2), LV_BUTTONMATRIX_CTRL_WIDTH_6, (lv_buttonmatrix_ctrl_t)(LV_BUTTONMATRIX_CTRL_HIDDEN | LV_BUTTONMATRIX_CTRL_WIDTH_2), LV_BUTTONMATRIX_CTRL_WIDTH_2
                                                    };

    /*Create a keyboard and add the new map as USER_1 mode*/
    kb = lv_keyboard_create(lv_screen_active());
    lv_obj_add_event_cb(kb, kb_num_event_cb, LV_EVENT_ALL, NULL);

    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, kb_map, kb_ctrl);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);

    // Apply style with reduced sensitivity
    static lv_style_t keyboards_style;
    lv_style_init(&keyboards_style);
    lv_style_set_rotary_sensitivity(&keyboards_style, 64);  // 0.25x sensitivity
    lv_obj_add_style(kb, &keyboards_style, 0);
    lv_group_add_obj(group, kb);


    
    ta1 = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(ta1, 240, 80);
    lv_obj_align(ta1, LV_ALIGN_TOP_MID, 0, 0);
    lv_textarea_set_placeholder_text(ta1, "Received Data");
    //lv_textarea_set_text(ta1, "");
    // lv_textarea_set_cursor_pos(ta1, true);
    // lv_textarea_set_one_line(ta1, false);
    //lv_textarea_set_max_length(ta1, RX_BUF_SIZE - 1);


    /*Create a text area. The keyboard will write here*/

    ta = lv_textarea_create(lv_screen_active());
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_size(ta, lv_pct(90), 80);
    //lv_obj_add_state(ta, LV_STATE_FOCUSED);

    lv_keyboard_set_textarea(kb, ta);
    //printf("textarea : %p\n", ta);
}
#endif