#pragma once

#define TASK_DISABLED 0
#define TASK_ENABLED  1

#define UART_TASK_INDEX     0
#define DISP_TASK_INDEX     1
#define WD_TASK_INDEX       2

#include "hardware/sync.h"
#include "pico/time.h"
#include <stdint.h>

typedef int (*task_fn)(void);
typedef int (*task_init_fn)(void);

typedef struct task_ctx {
  absolute_time_t deadline;
  uint32_t timeout_ms;

  char *name;

  task_fn realise;
  task_init_fn init;

  volatile uint8_t task_en;
} task_ctx_t;

extern task_ctx_t *tasks;
extern volatile bool should_wake_up;

inline void
disable_task(task_ctx_t *task) 
{
  task->task_en = TASK_DISABLED;
}

inline void
enable_task(task_ctx_t *task) 
{
  task->task_en = TASK_ENABLED;
  should_wake_up = true;
}

void __run_sched(void);
int add_task(task_ctx_t *task);
uint32_t get_task_count(void);
