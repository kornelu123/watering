#include <stdio.h>
#include <time.h>

#include "sched.h"

int
uart_task(void)
{
  printf("Time since boot: %us\n", to_ms_since_boot(get_absolute_time()));

  return 3000;
}
