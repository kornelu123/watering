#pragma once

typedef struct shared_mem {
  uint8_t running_slot_id;
} shared_mem_t;

// Memory is initialised as zeros by default
extern shared_mem_t *_shared;

static inline uint8_t
get_running_slot_id(void)
{
  return (_shared->running_slot_id);
}
