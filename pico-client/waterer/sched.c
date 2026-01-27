#include <stdio.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "sched.h"

#include "uart_task.c"
#include "disp_task.c"
#include "wd_task.c"

#define MIN_SCHED_TIMEOUT_MS 10000

static uint32_t task_count = 0;
task_ctx_t *tasks;

uint32_t
get_task_count(void) {
  return task_count;
}

  static inline task_ctx_t
init_task(const uint32_t timeout_ms, const char *name, task_fn task_function, task_init_fn init_function) 
{
  task_ctx_t task;

  task.timeout_ms = timeout_ms;
  task.realise = task_function;
  task.init = init_function;

  snprintf(task.name, 16, "%16s", name);

  return task;
}

  int
init_tasks(void)
{
  task_ctx_t uart_task_ctx = init_task(1000, "Uart task", &uart_task, NULL);
  add_task(&uart_task_ctx);

  task_ctx_t disp_task_ctx = init_task(10000, "Display task", &disp_task, &disp_init);
  add_task(&disp_task_ctx);

  task_ctx_t wd_task_ctx = init_task(4000, "Watchdog task", &wd_task, &wd_init);
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

  void
__run_sched(void)
{
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
      const absolute_time_t cur_time = get_absolute_time();

      if (cur_time >= tasks[i].deadline) {
        const int ret = tasks[i].realise();

        if (ret > 0) {
          tasks[i].timeout_ms = (uint32_t)ret;
        } else {
          printf("Task : %s failed with code : %d\n", tasks[i].name, ret);
        }
        tasks[i].deadline = make_timeout_time_ms(tasks[i].timeout_ms);
      }

      if (tasks[i].deadline < min_deadline) {
        min_deadline = tasks[i].deadline;
      }
    }

    const absolute_time_t after_time = get_absolute_time();
    if (absolute_time_min(min_deadline, after_time) != after_time) {
      sleep_until(min_deadline);
    }
  }
}
