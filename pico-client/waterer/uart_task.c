#include <stdio.h>

#include "sched.h"

int
uart_task(void)
{
  printf("hello from sched!\n");

  return 3000;
}
