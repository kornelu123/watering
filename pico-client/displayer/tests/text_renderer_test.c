#include "text_renderer.h"

#include <assert.h>
#include <stddef.h>

void set_pixel(int x, int y);

static size_t
black_pixel_count(void)
{
  size_t count = 0;

  for (size_t i = 0; i < sizeof(framebuf); ++i) {
    for (unsigned bit = 0; bit < 8; ++bit) {
      if ((framebuf[i] & (1u << bit)) == 0) {
        ++count;
      }
    }
  }

  return count;
}

int
main(void)
{
  clear_buffer();
  set_pixel(0, 0);
  assert(black_pixel_count() == 1);

  clear_buffer();
  set_pixel(0, EPD_HEIGHT - 1);
  assert(black_pixel_count() == 0);

  clear_buffer();
  set_pixel(EPD_WIDTH - 1, EPD_HEIGHT - 1);
  assert(black_pixel_count() == 0);

  clear_buffer();
  draw_string(-1, -1, "A");
  assert(black_pixel_count() > 0);

  return 0;
}
