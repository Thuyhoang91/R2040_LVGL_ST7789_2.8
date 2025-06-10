// File: lv_port_disp.h

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#ifdef __cplusplus
extern "C" {
#endif

// Define screen dimensions for portrait mode
#define LCD_WIDTH  240
#define LCD_HEIGHT 320
// Define the rotation for the display
#define LV_PORT_DISP_ROTATION_VALUE     1         // Landscape mode, USB port on the right
 // Portrait mode
// Define the rotation enum
typedef enum {
    LV_PORT_ROT_NONE = 0,   // Portrait mode
    LV_PORT_ROT_90,         // Landscape mode, USB port on the right
    LV_PORT_ROT_180,        // Inverted Portrait mode, USB port at the top
    LV_PORT_ROT_270         // Inverted Landscape mode, USB port on the left
} LV_PORT_DISP_ROTATION;

// The public function to initialize the display port
void lv_port_disp_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // LV_PORT_DISP_H
