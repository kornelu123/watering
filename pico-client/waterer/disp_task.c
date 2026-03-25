#include <stdio.h>
#include <time.h>

#include "pico/unique_id.h"

#include "ssd1306.c"

#define UPDATE_COUNT 5

#define SCREEN_DELTA_MS 1000

const char *pico_name;

static uint8_t update_count = 0;

int
disp_task(void)
{
  if (update_count == 0) {
    init_qled();
  }

  if (update_count >= UPDATE_COUNT) {
    update_count = 0;
    SSD1306_disable();
    return -1;
  } else {
    update_count++;
  }

  const uint32_t time_s = to_ms_since_boot(get_absolute_time())/1000;

  char text[16];
  if (time_s < 60) {
    snprintf(text, 16, "0000:00:%02u", time_s);
  } else if (time_s < 3600) {
    snprintf(text, 16, "0000:%02u:%02u", time_s/60, time_s%60);
  } else {
    snprintf(text, 16, "%04u:%02u:%02u", time_s/3600, (time_s%3600), time_s%60);
  }

  char *id;
  pico_get_unique_board_id_string(id, 16);
  set_string(id, 0);
  set_string("Since boot:      ", 1);
  set_string(text, 2);

  draw();

  return SCREEN_DELTA_MS;
}

int
disp_init(void)
{
  init_qled();

  return 3000;
}
