#include <stdint.h>

#include "hardware/watchdog.h"

static inline void
reset_pico(void)
{
  watchdog_reboot(0, 0, 0);

  while (true) {
    tight_loop_contents();
  }

  assert(false); // We should never be here
}
