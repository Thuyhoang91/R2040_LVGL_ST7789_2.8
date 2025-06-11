#include "lvgl.h"
#include <stdio.h>



// Forward declaration to fix implicit declaration error
void create_main_screen(void);
void create_jog_screen(void);
void create_file_screen(void);
void create_console_screen(void);
void create_settings_screen(void);

// console/terminal screen
static lv_obj_t * log_area;
static lv_obj_t * input_area;

//setings screen
static lv_obj_t * spindle_label;

// create jogging and feed rate labels
static lv_obj_t * step_label;
static lv_obj_t * feed_label;


// Global variables for file list and progress bar
static lv_obj_t * file_list;
static lv_obj_t * progress_bar;

static int step_size = 1;
static int feed_rate = 100;

// Event callback for return button in console

static void console_btn_event_cb(lv_event_t * e){
    // Return to main screen
    create_main_screen();
}
// Event callback for send button in console

static void send_btn_event_cb(lv_event_t * e) {
    const char * cmd = lv_textarea_get_text(input_area);
    if (strlen(cmd) == 0) return;

    // Simulate sending command to CNC controller
    char buf[128];
    snprintf(buf, sizeof(buf), "> %s\nOK\n", cmd);
    lv_label_set_text_fmt(log_area, "%s%s", lv_label_get_text(log_area), buf);

    // Clear input
    lv_textarea_set_text(input_area, "");
}

// Event callback for spindle speed slider

static void spindle_slider_event_cb(lv_event_t * e) {
    int speed = lv_slider_get_value(e->target);
    char buf[32];
    sprintf(buf, "Spindle: %d RPM", speed);
    lv_label_set_text(spindle_label, buf);
    // Send spindle speed command to CNC controller
}

static void coolant_switch_event_cb(lv_event_t * e) {
    bool state = lv_obj_has_state(e->target, LV_STATE_CHECKED);
    LV_LOG_USER("Coolant: %s", state ? "ON" : "OFF");
    // Send coolant on/off command
}

static void action_btn_event_cb(lv_event_t * e) {
    const char * action = lv_label_get_text(lv_obj_get_child(e->target, 0));
    LV_LOG_USER("Action: %s", action);
    // Send homing or zeroing command
    if (strcmp(action, "Home All") == 0) {
        // Send home all command
        LV_LOG_USER("Homing all axes");
    } else if (strcmp(action, "Zero XYZ") == 0) {
        // Send zero XYZ command
        LV_LOG_USER("Zeroing XYZ axes");
    } else if (strcmp(action, "Return") == 0) {
        create_main_screen(); // Return to main screen
    }
}

// Event callback for file selection
static void file_select_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    const char * file_name = lv_label_get_text(lv_obj_get_child(btn, 0));
    LV_LOG_USER("Selected file: %s", file_name);
    // Store selected file name or load preview
}
// Event callback for control buttons (Load, Start, Stop)
static void control_btn_event_cb(lv_event_t * e) {
    const char * action = lv_label_get_text(lv_obj_get_child(e->target, 0));
    LV_LOG_USER("Action: %s", action);

    if (strcmp(action, "Start") == 0) {
        // Start job logic
        lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    } else if (strcmp(action, "Stop") == 0) {
        // Stop job logic
        lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    } else if (strcmp(action, "Load") == 0) {
        // Load selected file logic
    }
    else if (strcmp(action, "Return") == 0) {
        create_main_screen(); // Return to main screen
    }
}

// Event callback for jog buttons
static void jog_btn_event_cb(lv_event_t * e) {
    const char * dir = lv_label_get_text(lv_obj_get_child(e->target, 0));
    LV_LOG_USER("Jog %s | Step: %dmm | Feed: %dmm/min", dir, step_size, feed_rate);
    // Send jog command to CNC controller here
    if (strcmp(dir, "X-") == 0) {
        // Send command for X-
    } else if (strcmp(dir, "X+") == 0) {
        // Send command for X+
    } else if (strcmp(dir, "Y-") == 0) {
        // Send command for Y-
    } else if (strcmp(dir, "Y+") == 0) {
        // Send command for Y+
    } else if (strcmp(dir, "Z-") == 0) {
        // Send command for Z-
    } else if (strcmp(dir, "Z+") == 0) {
        // Send command for Z+
    } else if (strcmp(dir, "Return") == 0) {
        // Handle return action
        create_main_screen(); // Return to main screen
    }
}

static void step_btn_event_cb(lv_event_t * e) {
    step_size *= 10;
    if (step_size > 10) step_size = 1;
    char buf[32];
    sprintf(buf, "Step: %dmm", step_size);
    lv_label_set_text(step_label, buf);
}

static void feed_slider_event_cb(lv_event_t * e) {
    feed_rate = lv_slider_get_value(e->target);
    char buf[32];
    sprintf(buf, "Feed: %dmm/min", feed_rate);
    lv_label_set_text(feed_label, buf);
}

static void emergency_stop_event_cb(lv_event_t * e) {
    // Handle emergency stop logic here
    LV_LOG_USER("Emergency Stop Pressed!");
}



static void nav_btn_event_cb(lv_event_t * e) {
    const char * btn_txt = lv_label_get_text(lv_obj_get_child(e->target, 0));
    LV_LOG_USER("Navigating to: %s", btn_txt);
    // Add screen switching logic here
    if (strcmp(btn_txt, "Jog") == 0) {
        create_jog_screen();
    } else if (strcmp(btn_txt, "Files") == 0) {
        create_file_screen();
    } else if (strcmp(btn_txt, "Settings") == 0) {
        create_settings_screen();
    } else if (strcmp(btn_txt, "Console") == 0) {
        create_console_screen();
    }
}


//////////////

// Functions 

// Function to create the settings screen

void create_console_screen(void) {
    // Clear the current screen
    lv_obj_clean(lv_scr_act()); // Clear current screen
    lv_obj_t * scr = lv_scr_act();

    // Title
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Console / Terminal");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Log Area
    lv_obj_t * cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 300, 150);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_ACTIVE);

    log_area = lv_label_create(cont);
    lv_label_set_long_mode(log_area, LV_LABEL_LONG_WRAP);
    lv_label_set_text(log_area, "");
    lv_obj_set_width(log_area, 280);
    lv_obj_align(log_area, LV_ALIGN_TOP_LEFT, 0, 0);

    // Input Area
    input_area = lv_textarea_create(scr);
    lv_obj_set_size(input_area, 220, 40);
    lv_obj_align(input_area, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_textarea_set_placeholder_text(input_area, "Enter G-code...");

    // Send Button
    lv_obj_t * send_btn = lv_btn_create(scr);
    lv_obj_set_size(send_btn, 80, 40);
    lv_obj_align(send_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_event_cb(send_btn, send_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(send_btn);
    lv_label_set_text(label, "Send");

    // add return button
    lv_obj_t * return_btn = lv_btn_create(scr);
    lv_obj_set_size(return_btn, 80, 40);
    lv_obj_align(return_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(return_btn, console_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * return_label = lv_label_create(return_btn);
    lv_label_set_text(return_label, "Return");

}

void create_settings_screen(void) {
    // Clear the current screen
    lv_obj_clean(lv_scr_act()); // Clear current screen
    lv_obj_t * scr = lv_scr_act();

    // Title
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Spindle Speed Slider
    lv_obj_t * slider = lv_slider_create(scr);
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_slider_set_range(slider, 0, 24000);
    lv_slider_set_value(slider, 12000, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, spindle_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    spindle_label = lv_label_create(scr);
    lv_label_set_text(spindle_label, "Spindle: 12000 RPM");
    lv_obj_align_to(spindle_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // Coolant Switch
    lv_obj_t * switch_label = lv_label_create(scr);
    lv_label_set_text(switch_label, "Coolant:");
    lv_obj_align(switch_label, LV_ALIGN_TOP_RIGHT, -100, 50);

    lv_obj_t * coolant_switch = lv_switch_create(scr);
    lv_obj_align_to(coolant_switch, switch_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(coolant_switch, coolant_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Action Buttons
    const char * actions[] = {"Home All", "Zero XYZ", "Return"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t * btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 100, 40);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 10 + i * 120, -20);
        lv_obj_add_event_cb(btn, action_btn_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, actions[i]);
    }
}

void create_file_screen(void) {
    // Clear the current screen
    lv_obj_clean(lv_scr_act()); // Clear current screen
    lv_obj_t * scr = lv_scr_act();

    // Title
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "File Manager");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // File List
    file_list = lv_list_create(scr);
    lv_obj_set_size(file_list, 200, 150);
    lv_obj_align(file_list, LV_ALIGN_TOP_LEFT, 10, 40);

    // Example files
    const char * files[] = {"part1.gcode", "part2.gcode", "test.nc", "demo.tap"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t * btn = lv_list_add_btn(file_list, LV_SYMBOL_FILE, files[i]);
        lv_obj_add_event_cb(btn, file_select_event_cb, LV_EVENT_CLICKED, NULL);
    }

    // Control Buttons
    const char * actions[] = {"Load", "Start", "Stop" , "Return"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t * btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 80, 30);
        lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -10, i * 50);
        lv_obj_add_event_cb(btn, control_btn_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, actions[i]);
    }

    // Progress Bar
    progress_bar = lv_bar_create(scr);
    lv_obj_set_size(progress_bar, 280, 20);
    lv_obj_align(progress_bar, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
}

void create_jog_screen(void) {
    // add clear screen logic if needed
    lv_obj_clean(lv_scr_act()); // Clear current screen
    lv_obj_t * scr = lv_scr_act();

    // Title
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Jog Control");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Step Size Button
    lv_obj_t * step_btn = lv_btn_create(scr);
    lv_obj_set_size(step_btn, 100, 40);
    lv_obj_align(step_btn, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_add_event_cb(step_btn, step_btn_event_cb, LV_EVENT_CLICKED, NULL);

    step_label = lv_label_create(step_btn);
    lv_label_set_text(step_label, "Step: 1mm");

    // Feed Rate Slider
    lv_obj_t * slider = lv_slider_create(scr);
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_TOP_RIGHT, -10, 50);
    lv_slider_set_range(slider, 10, 1000);
    lv_slider_set_value(slider, feed_rate, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, feed_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    feed_label = lv_label_create(scr);
    lv_label_set_text(feed_label, "Feed: 100mm/min");
    lv_obj_align_to(feed_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // Jog Buttons and screen return buttons
    
    //const char * directions[] = {"X-", "X+", "Y-", "Y+", "Z-", "Z+"}; // aad return directions  
    const char * directions[] = {"X-", "X+", "Y-", "Y+", "Z-", "Z+" , "Return"};    
    

    lv_coord_t x_offsets[] = {-60, 60, -60, 60, -60, 60, 120};
    lv_coord_t y_offsets[] = {0, 0, 40, 40, 120, 120, -100};

    for (int i = 0; i < 7; i++) {
        lv_obj_t * btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 60, 40);
        lv_obj_align(btn, LV_ALIGN_CENTER, x_offsets[i], y_offsets[i]);
        lv_obj_add_event_cb(btn, jog_btn_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, directions[i]);
    }
}


void create_main_screen(void) {
    // Clear the current screen
    lv_obj_clean(lv_scr_act()); // Clear current screen
    lv_obj_t * scr = lv_scr_act();

    // Title
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "CNC Controller");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Status Label
    lv_obj_t * status = lv_label_create(scr);
    lv_label_set_text(status, "Status: Idle");
    lv_obj_align(status, LV_ALIGN_TOP_LEFT, 10, 40);

    // Emergency Stop Button
    lv_obj_t * estop_btn = lv_btn_create(scr);
    lv_obj_set_size(estop_btn, 160, 50);
    lv_obj_align(estop_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(estop_btn, emergency_stop_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * estop_label = lv_label_create(estop_btn);
    lv_label_set_text(estop_label, "EMERGENCY STOP");

    // Navigation Buttons
    const char * nav_labels[] = {"Jog", "Files", "Settings", "Console"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t * btn = lv_btn_create(scr);
        lv_obj_set_size(btn, 80, 40);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 10 + i * 90, 80);
        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, nav_labels[i]);
    }
}
