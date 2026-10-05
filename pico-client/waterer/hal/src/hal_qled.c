#include "hal_qled.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "font.h"

#if defined(USE_SSD1306)
  #include "driver_ssd1306.c"
#else
    #error "Display not specified"
#endif

static char
text_buf[DISPLAY_HEIGHT/8][DISPLAY_WIDTH] = {
    "          ",
    "          ",
    "          ",
    "          ",
};

typedef struct {
    uint8_t x_start;
    uint8_t x_end;
    uint8_t y_start;
    uint8_t y_end;
} hal_qled_area_t;

static hal_qled_area_t frame_area = {
  .x_start = 0,
  .x_end   = DISPLAY_WIDTH - 1,
  .y_start = 0,
  .y_end   = DISPLAY_HEIGHT - 1,
};

static uint8_t buf[DISPLAY_BUF_LEN];

void
hal_qled_disable(void)
{
  display_driver_disable();
}

static void
set_pixel(uint8_t *buf, int x,int y, bool on)
{
  assert(x >= 0 && x < DISPLAY_WIDTH && y >=0 && y < DISPLAY_HEIGHT);

  // The calculation to determine the correct bit to set depends on which address
  // mode we are in. This code assumes horizontal

  // The video ram on the SSD1306 is split up in to 8 rows, one bit per pixel.
  // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x=0, y=0->
  // byte 1 is x = 1, y=0->7 etc

  // This code could be optimised, but is like this for clarity. The compiler
  // should do a half decent job optimising it anyway.

  const int BytesPerRow = DISPLAY_WIDTH; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

  int byte_idx = (y / 8) * BytesPerRow + x;
  uint8_t byte = buf[byte_idx];

  if (on)
    byte |=  1 << (y % 8);
  else
    byte &= ~(1 << (y % 8));

  buf[byte_idx] = byte;
}

// Basic Bresenhams.
  static void
draw_line(uint8_t *buf, int x0, int y0, int x1, int y1, bool on) 
{
  int dx =  abs(x1-x0);
  int sx = x0<x1 ? 1 : -1;
  int dy = -abs(y1-y0);
  int sy = y0<y1 ? 1 : -1;
  int err = dx+dy;
  int e2;

  while (true) {
    set_pixel(buf, x0, y0, on);
    if (x0 == x1 && y0 == y1)
      break;
    e2 = 2*err;

    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

  static inline int
get_font_index(uint8_t ch)
{
  if (ch >= 'A' && ch <='Z') {
    return  ch - 'A' + 1;
  }
  else if (ch >= '0' && ch <='9') {
    return  ch - '0' + 27;
  }
  else return  0; // Not got that char so space.
}

  void
write_char(uint8_t *buf, int16_t x, int16_t y, uint8_t ch)
{
  if (x > DISPLAY_WIDTH - 8 || y > DISPLAY_HEIGHT - 8)
    return;

  // For the moment, only write on Y row boundaries (every 8 vertical pixels)
  y = y/8;

  ch = toupper(ch);
  int idx = get_font_index(ch);
  int fb_idx = y * DISPLAY_WIDTH + x;

  for (int i=0;i<8;i++) {
    buf[fb_idx++] = font[idx * 8 + i];
  }
}

  void
write_string(uint8_t *buf, int16_t x, int16_t y, char *str)
{
  // Cull out any string off the screen
  if (x > DISPLAY_WIDTH - 8 || y > DISPLAY_HEIGHT - 8)
    return;

  while (*str) {
    write_char(buf, x, y, *str++);
    x+=8;
  }
}

int
hal_qled_init(void)
{
  display_driver_init();
  display_driver_render(buf, frame_area.x_start, frame_area.x_end, frame_area.y_start, frame_area.y_end);

  int y = 0;
  for (uint32_t i = 0 ;i < (DISPLAY_HEIGHT / 8); i++) {
    write_string(buf, 0, y, text_buf[i]);
    y+=8;
  }

  display_driver_render(buf, frame_area.x_start, frame_area.x_end, frame_area.y_start, frame_area.y_end);

  return 0;
}

void
hal_qled_set_string(const char *text, const uint8_t column)
{
  assert(column <= 3);

  memcpy(&(text_buf[column]), text, 16);
}

void
hal_qled_draw(void)
{
  for (uint32_t i = 0 ;i < 4; i++) {
    write_string(buf, 5, i*8, text_buf[i]);
  }

  display_driver_render(buf, frame_area.x_start, frame_area.x_end, frame_area.y_start, frame_area.y_end);
}