#pragma once

#include "pico/time.h"
#include <stdint.h>

typedef int (*task_fn)(void);
typedef int (*task_init_fn)(void);

typedef struct task_ctx {
  absolute_time_t deadline;
  uint32_t timeout_ms;

  char name[16];

  task_fn realise;
  task_init_fn init;
} task_ctx_t;

extern task_ctx_t *tasks;

void __run_sched(void);
int add_task(task_ctx_t *task);
uint32_t get_task_count(void);
