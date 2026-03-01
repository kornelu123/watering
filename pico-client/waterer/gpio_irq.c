#include "hardware/irq.h"

#include <stdio.h>

#include "sched.h"
#include "pins.h"

void gpio_callback(uint gpio, uint32_t events) {
  switch (gpio) {
    case GPIO_BUTTON_IRQ_PIN: 
      {
        enable_task(&tasks[DISP_TASK_INDEX]);
        break;
      }
    default:
      break;
  }
}

void
init_gpio(void)
{
  gpio_init(GPIO_BUTTON_IRQ_PIN);

  gpio_set_irq_enabled_with_callback(GPIO_BUTTON_IRQ_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
}
