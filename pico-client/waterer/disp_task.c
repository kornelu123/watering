#include <stdio.h>
#include <time.h>

#include "ssd1306.c"

int
disp_task(void)
{
  char text[18];

  const uint32_t time_s = to_ms_since_boot(get_absolute_time())/1000;

  if (time_s < 60) {
    snprintf(text, 16, "   00:00:%2u ", time_s);
  } else if (time_s < 3600) {
    snprintf(text, 16, "   00:%2u:%2u ", time_s/60, time_s%60);
  } else {
    snprintf(text, 16, "   %4u:%2u:%2u ", time_s/3600, (time_s%3600), time_s%60);
  }

  set_string("   Since boot:", 0);
  set_string(text, 2);

  draw();

  return 5000;
}

int
disp_init(void)
{
  init_qled();
  printf("hello from sched!\n");

  return 3000;
}
