#include "gui.h"
#include "lv_port_disp.h" // For LCD_WIDTH/HEIGHT
#include <stdio.h>
#include <string.h>
#include "lvgl.h"

// --- Mock GRBL Data Structure ---
// We define this locally now since we are not using grbl_comm.h
typedef struct {
    char state[16];
    float wpos[3];
} grbl_mock_status_t;

// --- Module-level variables for simulation ---
static grbl_mock_status_t mock_status;
static lv_obj_t *label_status;
static lv_obj_t *label_wpos_x;
static lv_obj_t *label_wpos_y;
static lv_obj_t *label_wpos_z;


// --- MOCK FUNCTION ---
// This function simulates sending a command by printing it to the console.
static void mock_grbl_send_command(const char* cmd) {
    printf("GUI ACTION: Would send command -> [%s]\n", cmd);
}

// --- Event handler for all control buttons (now calls the mock function) ---
static void control_btn_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        const char* cmd = (const char*)lv_event_get_user_data(e);
        if (cmd) {
            mock_grbl_send_command(cmd);
        }
    }
}

// --- Helper to create a standard control button ---
static lv_obj_t* create_control_btn(lv_obj_t *parent, const char *text, const char *grbl_cmd) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_event_cb(btn, control_btn_event_handler, LV_EVENT_CLICKED, (void*)grbl_cmd);
    
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    return btn;
}


// --- Data Simulation Timer ---
// This timer callback runs periodically to generate fake data, making the UI feel alive.
static void data_simulation_timer_cb(lv_timer_t *timer) {
    static float x = 10.123, y = 25.456, z = 5.789;
    static int state_idx = 0;
    const char* states[] = {"Idle", "Run", "Hold:0"};

    // Update coordinates
    x += 0.03f;
    y += 0.05f;
    z -= 0.01f;
    if (x > 100.0f) x = 10.0f;
    if (y > 100.0f) y = 25.0f;
    if (z < 0.0f) z = 5.0f;
    
    mock_status.wpos[0] = x;
    mock_status.wpos[1] = y;
    mock_status.wpos[2] = z;

    // Cycle through states
    state_idx = (state_idx + 1) % 30; // Change state every ~3 seconds
    if (state_idx == 0) {
        strcpy(mock_status.state, states[1]); // Run
    } else if (state_idx == 15) {
        strcpy(mock_status.state, states[2]); // Hold
    } else if (state_idx == 20){
        strcpy(mock_status.state, states[0]); // Idle
    }
}


// --- UI Update Timer ---
// This timer reads the simulated data and updates the labels on the screen.
static void ui_update_timer_cb(lv_timer_t *timer) {
    // No mutex needed since everything is on Core 0
    lv_label_set_text(label_status, mock_status.state);
    lv_label_set_text_fmt(label_wpos_x, "X: %.3f", mock_status.wpos[0]);
    lv_label_set_text_fmt(label_wpos_y, "Y: %.3f", mock_status.wpos[1]);
    lv_label_set_text_fmt(label_wpos_z, "Z: %.3f", mock_status.wpos[2]);
}

// --- The main GUI creation function ---
void gui_create(void) {
    // Initialize mock data
    strcpy(mock_status.state, "Idle");
    mock_status.wpos[0] = 0.0f;
    mock_status.wpos[1] = 0.0f;
    mock_status.wpos[2] = 0.0f;

    // --- Create a header for status ---
    lv_obj_t* header = lv_obj_create(lv_scr_act());
    lv_obj_set_size(header, LCD_WIDTH, 40);
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_pad_all(header, 5, 0);
    label_status = lv_label_create(header);
    lv_label_set_text(label_status, "Connecting...");
    lv_obj_align(label_status, LV_ALIGN_LEFT_MID, 5, 0);
    
    // --- Create a footer for position ---
    lv_obj_t* footer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(footer, LCD_WIDTH, 40);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    label_wpos_x = lv_label_create(footer);
    label_wpos_y = lv_label_create(footer);
    label_wpos_z = lv_label_create(footer);
    lv_label_set_text(label_wpos_x, "X: 0.000");
    lv_label_set_text(label_wpos_y, "Y: 0.000");
    lv_label_set_text(label_wpos_z, "Z: 0.000");

    // --- Create the main Tab View ---
    lv_obj_t* tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 40);
    lv_obj_set_size(tabview, LCD_WIDTH, LCD_HEIGHT - 80);
    lv_obj_align(tabview, LV_ALIGN_TOP_LEFT, 0, 40);
    lv_obj_t *tab_jog = lv_tabview_add_tab(tabview, "Jog");
    lv_obj_t *tab_cmd = lv_tabview_add_tab(tabview, "Commands");
    // You can add more tabs like lv_tabview_add_tab(tabview, "File");

    // --- Populate the "Jog" Tab ---
    lv_obj_set_grid_dsc_array(tab_jog, (lv_coord_t[]){LV_GRID_FR(1), 70, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST},
                                       (lv_coord_t[]){LV_GRID_FR(1), 70, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST});
    lv_obj_t *btn_up = create_control_btn(tab_jog, LV_SYMBOL_UP, "$J=G91Y10F500");
    lv_obj_set_grid_cell(btn_up, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_t *btn_left = create_control_btn(tab_jog, LV_SYMBOL_LEFT, "$J=G91X-10F500");
    lv_obj_set_grid_cell(btn_left, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    lv_obj_t *btn_right = create_control_btn(tab_jog, LV_SYMBOL_RIGHT, "$J=G91X10F500");
    lv_obj_set_grid_cell(btn_right, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    lv_obj_t *btn_down = create_control_btn(tab_jog, LV_SYMBOL_DOWN, "$J=G91Y-10F500");
    lv_obj_set_grid_cell(btn_down, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 2, 1);
    
    // --- Populate the "Commands" Tab ---
    lv_obj_set_flex_flow(tab_cmd, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab_cmd, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(tab_cmd, 10, 0);
    create_control_btn(tab_cmd, "Home ($H)", "$H");
    create_control_btn(tab_cmd, "Unlock ($X)", "$X");
    create_control_btn(tab_cmd, "Set WCS Zero", "G10 L20 P1 X0 Y0 Z0");

    // --- Start the UI update and data simulation timers ---
    lv_timer_create(ui_update_timer_cb, 200, NULL);       // Updates the screen labels
    lv_timer_create(data_simulation_timer_cb, 100, NULL); // Generates fake data
}
