#include "hardware/watchdog.h"

#include <stdio.h>

#define WATCHDOG_TIMEOUT_MS 8000
#define WATCHDOG_UPDATE_MS  4000

int
wd_task(void)
{
  watchdog_update();

  return WATCHDOG_UPDATE_MS;
}

int
wd_init(void)
{
  watchdog_enable(WATCHDOG_TIMEOUT_MS, true);

  return 0;
}
