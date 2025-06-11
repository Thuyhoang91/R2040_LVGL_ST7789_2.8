/**
 * @file cnc_gui.c
 * @brief Creates a complete CNC control GUI.
 *
 * Encapsulates all UI logic into a single file with one public function: gui_create().
 * All other functions and file-scope variables are static to this module.
 */

#include "lvgl.h"
#include <stdio.h>  // For printf, used in event handlers for demonstration
#include <stdlib.h> // For atof

//-------------------------------------
// STATIC VARIABLES (File-Scope)
//-------------------------------------

// Pointers to UI objects that need to be updated dynamically
static lv_obj_t *label_dro_x;
static lv_obj_t *label_dro_y;
static lv_obj_t *label_dro_z;
static lv_obj_t *label_status;
static lv_obj_t *job_progress_bar;
static lv_obj_t *job_elapsed_label;
static lv_obj_t *job_modal_overlay = NULL; // Pointer to the modal screen

// Global state variables for simulation
static float current_step = 1.0f;
static int job_elapsed_seconds = 0;

// Track the last selected file button in the file list
static lv_obj_t* last_selected_file_btn = NULL;

//-------------------------------------
// STATIC FUNCTION PROTOTYPES
//-------------------------------------

// GUI Builder Functions
static void create_control_tab(lv_obj_t * parent);
static void create_file_tab(lv_obj_t * parent);
static void create_run_job_modal(const char* filename);

// Event Handlers
static void generic_button_event_handler(lv_event_t * e);
static void jog_panel_event_handler(lv_event_t * e);
static void step_select_event_handler(lv_event_t * e);
static void spindle_event_handler(lv_event_t * e);
static void stop_job_event_handler(lv_event_t * e);
static void file_list_event_handler(lv_event_t * e);
static void load_run_event_handler(lv_event_t * e);

// Timer Callback for Simulation
static void simulation_updater_timer(lv_timer_t * timer);


//-------------------------------------
// EVENT HANDLERS
//-------------------------------------

static void generic_button_event_handler(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    printf("Clicked: %s\n", lv_label_get_text(label));
}

static void jog_panel_event_handler(lv_event_t * e) {
    lv_obj_t * btnm = lv_event_get_target(e);
    uint32_t id = lv_btnmatrix_get_selected_btn(btnm);
    const char * txt = lv_btnmatrix_get_btn_text(btnm, id);
    if(txt) printf("Jog panel: %s (Step: %.2f)\n", txt, current_step);
}

static void step_select_event_handler(lv_event_t * e) {
    lv_obj_t * btnm = lv_event_get_target(e);
    uint32_t id = lv_btnmatrix_get_selected_btn(btnm);
    const char * txt = lv_btnmatrix_get_btn_text(btnm, id);
    if(txt) {
        current_step = atof(txt);
        printf("Step size set to: %.2f\n", current_step);
    }
}

static void spindle_event_handler(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    if(lv_obj_has_state(btn, LV_STATE_CHECKED)) {
        lv_label_set_text(label, "Spindle\nON");
        printf("Spindle ON\n");
    } else {
        lv_label_set_text(label, "Spindle\nOFF");
        printf("Spindle OFF\n");
    }
}

static void stop_job_event_handler(lv_event_t * e) {
    printf("Job stopped!\n");
    if(job_modal_overlay) {
        lv_obj_del(job_modal_overlay);
        job_modal_overlay = NULL;
    }
}

static void file_list_event_handler(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    last_selected_file_btn = btn;
    printf("Selected file: %s\n", lv_list_get_btn_text(lv_obj_get_parent(btn), btn));
}

static void load_run_event_handler(lv_event_t * e) {
    lv_obj_t* list = (lv_obj_t*)lv_event_get_user_data(e);
    if(last_selected_file_btn && lv_obj_get_parent(last_selected_file_btn) == list) {
        const char* filename = lv_list_get_btn_text(list, last_selected_file_btn);
        printf("Loading and running: %s\n", filename);
        create_run_job_modal(filename);
    } else {
        printf("No file selected!\n");
    }
}

//-------------------------------------
// GUI BUILDER FUNCTIONS
//-------------------------------------

static void create_control_tab(lv_obj_t * parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {160, 100, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);

    // --- DRO Panel ---
    lv_obj_t * dro_panel = lv_obj_create(parent);
    lv_obj_set_grid_cell(dro_panel, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_flex_flow(dro_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(dro_panel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    label_dro_x = lv_label_create(dro_panel);
    label_dro_y = lv_label_create(dro_panel);
    label_dro_z = lv_label_create(dro_panel);

    // --- Jog Panel ---
    lv_obj_t * jog_panel = lv_obj_create(parent);
    lv_obj_set_grid_cell(jog_panel, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    
    static const char * jog_map[] = {LV_SYMBOL_UP, "\n", LV_SYMBOL_LEFT, "HOME", LV_SYMBOL_RIGHT, "\n", LV_SYMBOL_DOWN, ""};
    lv_obj_t * jog_btnm = lv_btnmatrix_create(jog_panel);
    lv_btnmatrix_set_map(jog_btnm, jog_map);
    lv_obj_set_size(jog_btnm, 180, 130);
    lv_obj_center(jog_btnm);
    lv_obj_add_event_cb(jog_btnm, jog_panel_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Step/Z Panel ---
    lv_obj_t * step_z_panel = lv_obj_create(parent);
    lv_obj_set_grid_cell(step_z_panel, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_layout(step_z_panel, LV_LAYOUT_GRID);
    static lv_coord_t sz_col[] = {LV_GRID_FR(2), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t sz_row[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(step_z_panel, sz_col, sz_row);

    static const char * step_map[] = {"0.1", "1", "10", ""};
    lv_obj_t * step_btnm = lv_btnmatrix_create(step_z_panel);
    lv_obj_set_grid_cell(step_btnm, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_btnmatrix_set_map(step_btnm, step_map);
    lv_btnmatrix_set_one_checked(step_btnm, true);
    lv_btnmatrix_set_btn_ctrl(step_btnm, 1, LV_BTNMATRIX_CTRL_CHECKED);
    lv_obj_add_event_cb(step_btnm, step_select_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    lv_obj_t * z_cont = lv_obj_create(step_z_panel);
    lv_obj_set_grid_cell(z_cont, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_flex_flow(z_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(z_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* z_up_btn = lv_btn_create(z_cont); lv_obj_t* z_up_lbl = lv_label_create(z_up_btn); lv_label_set_text(z_up_lbl, "Z+"); lv_obj_add_event_cb(z_up_btn, generic_button_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_t* z_down_btn = lv_btn_create(z_cont); lv_obj_t* z_down_lbl = lv_label_create(z_down_btn); lv_label_set_text(z_down_lbl, "Z-"); lv_obj_add_event_cb(z_down_btn, generic_button_event_handler, LV_EVENT_CLICKED, NULL);

    // --- Spindle Panel ---
    lv_obj_t * spindle_panel = lv_obj_create(parent);
    lv_obj_set_grid_cell(spindle_panel, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_t* spindle_btn = lv_btn_create(spindle_panel);
    lv_obj_add_flag(spindle_btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_size(spindle_btn, 120, lv_pct(80));
    lv_obj_center(spindle_btn);
    lv_obj_t* spindle_lbl = lv_label_create(spindle_btn); lv_label_set_text(spindle_lbl, "Spindle\nOFF"); lv_obj_center(spindle_lbl);
    lv_obj_add_event_cb(spindle_btn, spindle_event_handler, LV_EVENT_CLICKED, NULL);
}

static void create_file_tab(lv_obj_t * parent) {
    lv_obj_t * list = lv_list_create(parent);
    lv_obj_set_size(list, lv_pct(95), lv_pct(75));
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 5);
    
    const char* files[] = {"box_lid.nc", "pcb_holes.gcode", "calibration.nc", "logo_final.gcode", NULL};
    for(int i = 0; files[i] != NULL; i++) {
        lv_obj_t* btn = lv_list_add_btn(list, LV_SYMBOL_FILE, files[i]);
        lv_obj_add_event_cb(btn, file_list_event_handler, LV_EVENT_CLICKED, NULL);
    }
    
    lv_obj_t* run_btn = lv_btn_create(parent);
    lv_obj_align(run_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_event_cb(run_btn, load_run_event_handler, LV_EVENT_CLICKED, list);
    lv_obj_t* run_lbl = lv_label_create(run_btn);
    lv_label_set_text(run_lbl, LV_SYMBOL_PLAY " Load & Run");
}

static void create_run_job_modal(const char* filename) {
    if(job_modal_overlay) return; // Prevent creating multiple modals

    job_modal_overlay = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(job_modal_overlay);
    lv_obj_set_size(job_modal_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(job_modal_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(job_modal_overlay, LV_OPA_50, 0);
    lv_obj_center(job_modal_overlay);

    lv_obj_t * cont = lv_obj_create(job_modal_overlay);
    lv_obj_set_size(cont, lv_pct(90), lv_pct(80));
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * title = lv_label_create(cont);
    lv_label_set_text_fmt(title, "Running: %s", filename);

    job_progress_bar = lv_bar_create(cont);
    lv_obj_set_width(job_progress_bar, lv_pct(90));
    lv_bar_set_value(job_progress_bar, 0, LV_ANIM_OFF);

    job_elapsed_label = lv_label_create(cont);
    job_elapsed_seconds = 0; // Reset timer

    lv_obj_t* stop_btn = lv_btn_create(cont);
    lv_obj_set_style_bg_color(stop_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(stop_btn, stop_job_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_t* stop_lbl = lv_label_create(stop_btn);
    lv_label_set_text(stop_lbl, LV_SYMBOL_STOP " STOP JOB");
}


//-------------------------------------
// SIMULATION TIMER
//-------------------------------------

static void simulation_updater_timer(lv_timer_t * timer) {
    // Update DRO labels with dummy data
    lv_label_set_text_fmt(label_dro_x, "X: %.3f", lv_rand(1000, 200000) / 1000.0f);
    lv_label_set_text_fmt(label_dro_y, "Y: %.3f", lv_rand(-50000, 50000) / 1000.0f);
    lv_label_set_text_fmt(label_dro_z, "Z: %.3f", lv_rand(0, 75000) / 1000.0f);

    // Update the job modal if it's visible
    if(job_modal_overlay) {
        job_elapsed_seconds++;
        int hours = job_elapsed_seconds / 3600;
        int mins = (job_elapsed_seconds % 3600) / 60;
        int secs = job_elapsed_seconds % 60;
        lv_label_set_text_fmt(job_elapsed_label, "Elapsed: %02d:%02d:%02d", hours, mins, secs);

        int current_progress = lv_bar_get_value(job_progress_bar);
        if(current_progress < 100) {
            lv_bar_set_value(job_progress_bar, current_progress + 1, LV_ANIM_ON);
        } else {
             // Simulate job completion
            stop_job_event_handler(NULL);
            lv_label_set_text(label_status, "Status: Job Done");
        }
    } else {
        // Update main status label
        const char *states[] = {"Idle", "Idle", "Idle", "Jogging..."};
        lv_label_set_text_fmt(label_status, "Status: %s", states[lv_rand(0,3)]);
    }
}


//-------------------------------------
// PUBLIC FUNCTION
//-------------------------------------

/**
 * @brief Creates the entire CNC GUI. This is the main entry point.
 */
void gui_create(void)
{
    lv_obj_t * tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);

    lv_obj_t * tab_control = lv_tabview_add_tab(tabview, "Control");
    lv_obj_t * tab_file = lv_tabview_add_tab(tabview, "File");
    lv_obj_t * tab_settings = lv_tabview_add_tab(tabview, "Settings");

    create_control_tab(tab_control);
    create_file_tab(tab_file);

    lv_label_set_text(lv_label_create(tab_settings), "Settings will be here.");
    
    label_status = lv_label_create(lv_scr_act());
    lv_obj_align(label_status, LV_ALIGN_TOP_RIGHT, -10, 10);

    lv_timer_create(simulation_updater_timer, 500, NULL);
}
