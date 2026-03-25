#include <stdio.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "sched.h"

#include "pico/sleep.h"

#include "hardware/pll.h"
#include "hardware/regs/clocks.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "hardware/xosc.h"
#include "hardware/rosc.h"
#include "hardware/regs/io_bank0.h"

#include "hardware/structs/scb.h"

#include "uart_task.c"
#include "disp_task.c"
#include "wd_task.c"

#define MIN_SCHED_TIMEOUT_MS 10000

task_ctx_t *tasks;

static uint32_t task_count = 0;
volatile bool should_wake_up = false;

  static void
alarm_sleep_callback(uint alarm_id)
{
  hardware_alarm_set_callback(alarm_id, NULL);
  hardware_alarm_unclaim(alarm_id);

  should_wake_up = true;
}

static void sched_go_sleep_until(absolute_time_t deadline)
{
  absolute_time_t now = get_absolute_time();
  if (deadline <= now) {
    return;
  }

  int alarm_num = hardware_alarm_claim_unused(true);
  if (alarm_num >= 0) {
    hardware_alarm_set_target(alarm_num, deadline);

    __wfe();

    hardware_alarm_unclaim(alarm_num);
  } else {
    while (get_absolute_time() < deadline) {
      tight_loop_contents();
    }
  }
}

uint32_t
get_task_count(void) {
  return task_count;
}

  static inline task_ctx_t
init_task(const uint32_t timeout_ms, const char *name, task_fn task_function, task_init_fn init_function, uint8_t index)
{
  task_ctx_t task;

  task.timeout_ms = timeout_ms;
  task.realise = task_function;
  task.init = init_function;

  enable_task(&task);

  return task;
}

  int
init_tasks(void)
{
  task_ctx_t uart_task_ctx = init_task(1000, "Uart task", &uart_task, NULL, UART_TASK_INDEX);
  add_task(&uart_task_ctx);

  task_ctx_t disp_task_ctx = init_task(500, "Display task", &disp_task, &disp_init, DISP_TASK_INDEX);
  add_task(&disp_task_ctx);

  task_ctx_t wd_task_ctx = init_task(4000, "Watchdog task", &wd_task, &wd_init, WD_TASK_INDEX);
  add_task(&wd_task_ctx);
}

  int
add_task(task_ctx_t *task)
{
  task_count += 1;

  tasks = (task_ctx_t *)reallocarray(tasks, task_count, sizeof(task_ctx_t));

  if (tasks == NULL) {
    return -1;
  }

  memcpy(&(tasks[task_count - 1]), task, sizeof(task_ctx_t));

  return 0;
}

  static void
init_irq(void)
{
  const uint32_t mask = 0x00000000 | (
          (1 << 11) |               // DMA_IRQ_0    enabled
          (1 << 12) |               // DMA_IRQ_1    enabled
          (1 << 13) |               // IO_IRQ_BANK0 enabled
          (1 << 14) |               // IO_IRQ_QSPI  enabled
          (1 << 25)                 // RTC_IRQ      enabled
        );

  irq_set_mask_enabled(mask, true);
}

  void
__run_sched(void)
{
  init_irq();
  init_tasks();

  for (int i=0; i<get_task_count(); i++) {
    tasks[i].deadline = get_absolute_time();
  }

  for (int i=0; i<task_count; i++) {
    if (tasks[i].init != NULL) {
      tasks[i].init();
    }
  }

  while (true) {
    absolute_time_t min_deadline = delayed_by_ms(get_absolute_time(), MIN_SCHED_TIMEOUT_MS);

    for (int i=0; i<get_task_count(); i++) {
      if (tasks[i].task_en != TASK_ENABLED) {
        continue;
      }

      const absolute_time_t cur_time = get_absolute_time();

      if (cur_time >= tasks[i].deadline) {
        const int ret = tasks[i].realise();

        if (ret > 0) {
          tasks[i].timeout_ms = (uint32_t)ret;
        } else if (ret == -1) {
          disable_task(&tasks[i]);
        } else {
        }
        tasks[i].deadline = make_timeout_time_ms(tasks[i].timeout_ms);
      }

      if (tasks[i].deadline < min_deadline) {
        min_deadline = tasks[i].deadline;
      }
    }

    sched_go_sleep_until(min_deadline);
  }
}
