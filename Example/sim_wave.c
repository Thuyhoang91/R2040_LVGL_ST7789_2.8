
#include "lvgl.h"  // Adjust path for your project
#include <math.h>               // For sinf and M_PI
#include "sim_wave.h"

#define CANVAS_WIDTH  150
#define CANVAS_HEIGHT 150
#define NUM_POINTS    200  // Number of samples for smooth curve
#define ANIMATION_INTERVAL_MS 5  // Update every 50ms (20 FPS)
// #define PHASE_INCREMENT 0.1f  // Phase shift per frame (radians)
#define BASE_PHASE_INCREMENT 0.2f  // Base phase shift per frame at 33ms
#define BASE_INTERVAL_MS 33.0f     // Reference interval for scaling phase



// Function to draw the sine wave
static void sine_osc_timer(lv_timer_t * timer)
{
    static float phase=0.0f;
    // // Debug counter to verify timer firing (optional)
    // static uint32_t frame_count = 0;
    // frame_count++;
    // Optionally log frame_count, e.g., printf("Frame: %u\n", frame_count);


    //sine_osc_t * data = (sine_osc_t *)timer->user_data;
    lv_obj_t * canvas =  (lv_obj_t *) lv_timer_get_user_data(timer);
    //float phase = data->phase;

    // Initialize layer for drawing
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    // Clear canvas (black background)
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    // Initialize line descriptor
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_hex(0x00FF00);  // Green
    line_dsc.width = 2;
    line_dsc.opa = LV_OPA_COVER;             // Fully opaque
    line_dsc.round_start = 1;                 // Rounded start
    line_dsc.round_end = 1;                   // Rounded end
    line_dsc.raw_end = 0;                     // Use anti-aliasing
    line_dsc.dash_width = 0;                  // Solid line
    line_dsc.dash_gap = 0;

    // Generate sine wave points with phase shift
    lv_point_precise_t p1, p2;
    for (uint32_t i = 0; i < NUM_POINTS - 1; i++) {
        float x = (float)i / (NUM_POINTS - 1);  // Normalize x to [0, 1]
        line_dsc.p1.x = (lv_coord_t)(CANVAS_WIDTH * x);  // Scale to canvas width
        line_dsc.p1.y = (lv_coord_t)((CANVAS_HEIGHT / 2) + (40 * sinf(x * 2 * M_PI * 2.5 + phase)));
        line_dsc.p2.x = (lv_coord_t)(CANVAS_WIDTH * ((float)(i + 1) / (NUM_POINTS - 1)));
        line_dsc.p2.y = (lv_coord_t)((CANVAS_HEIGHT / 2) + (40 * sinf(((float)(i + 1) / (NUM_POINTS - 1)) * 2 * M_PI * 2.5 + phase)));

        // Draw line segment
        lv_draw_line(&layer,&line_dsc);
    }
    
    // Finish layer and refresh
    lv_canvas_finish_layer(canvas, &layer);
    lv_obj_invalidate(canvas);


     // Update phase, scaled by interval to maintain consistent speed
    float phase_increment = BASE_PHASE_INCREMENT * (BASE_INTERVAL_MS / ANIMATION_INTERVAL_MS);
    // Update phase for next frame
    phase += phase_increment;
    if (phase >= 2 * M_PI) phase -= 2 * M_PI;  // Keep phase in [0, 2π]
}

void lv_example_sine_osc(void)
{
    // Define static draw buffer (RGB565 format)
    LV_DRAW_BUF_DEFINE_STATIC(draw_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_RGB565);
    LV_DRAW_BUF_INIT_STATIC(draw_buf);

    // Create canvas
    lv_obj_t * canvas = lv_canvas_create(lv_screen_active());
    lv_canvas_set_draw_buf(canvas, &draw_buf);
    lv_obj_set_size(canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
    lv_obj_center(canvas);

    // Initial draw
     lv_timer_create(sine_osc_timer, ANIMATION_INTERVAL_MS, canvas);
}


