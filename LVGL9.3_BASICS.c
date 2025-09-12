#include <stdio.h>
#include "pico/stdlib.h"
#include "lv_port.h"
#include "lvgl.h"
#include "st7789.h"
#include "ex1.h"

int main()
{
    stdio_init_all();
    st7789_init();    
    lv_port_init(); 
    //setup_ui();

    while (true) {
        lv_task_handler(); // Handle LVGL tasks
        tight_loop_contents(); // Idle loop
    }
    return 0;
}

