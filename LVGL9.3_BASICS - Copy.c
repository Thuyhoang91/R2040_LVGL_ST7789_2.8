#include <stdio.h>
#include "pico/stdlib.h"
#include "lv_port.h"
#include "lvgl.h"
#include "st7789.h"
#include "img_style.h"


extern lv_obj_t *ta1;
// // Ring Buffer for USB RX
#define BUF_SIZE 256
char buf[BUF_SIZE]; // Temporary buffer

// #include "img_style.h"
int main()
{
    stdio_init_all();
    st7789_init();
    lv_port_init(); 
    //kb_custom_map();
    kb_ta_map();

    // Wait for USB connection
    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }

    // Main loop
    // uint32_t last_wake = 0;
    // char line_buf[RING_BUF_SIZE];
    // uint line_len = 0;

    while (true) {        
        //get all characters from USB buffer
        int read_count = stdio_get_until(buf, BUF_SIZE, 0);
        if (read_count > 0) {
            // Null-terminate and display received data
            buf[read_count] = '\0';
            //add char /newline to textarea1
            lv_textarea_add_text(ta1, buf);
            printf("Received: %s\n", buf);
            lv_textarea_add_text(ta1, "\n");
        }
        //sleep_ms(100); // Small delay to avoid busy loop
        lv_task_handler(); // handle LVGL tasks
        tight_loop_contents();
    }
    return 0;
}
