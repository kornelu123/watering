#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <stdint.h>
#include <stdbool.h>

// Dimensions of the e-ink display
#define EPD_WIDTH    128
#define EPD_HEIGHT   250
#define X_BYTES      ((EPD_WIDTH + 7) / 8)
#define Y_LINES      (EPD_HEIGHT)

extern uint8_t framebuf[X_BYTES * Y_LINES];


// Clear frame buffer 
void clear_buffer(void);

// Draw a single charatcer
void draw_char(int x, int y, char c);

// Draw SKN MOS logo in the bottom of the EPD
void draw_logo();

// Draw a null-terminated string starting at (x0,y0) - supports '\n'
void draw_string(int x0, int y0, const char *s);

#endif
