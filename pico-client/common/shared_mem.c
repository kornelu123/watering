#include "shared_mem.h"

shared_mem_t __attribute__((section(".shared")))shared = {
  0,
  "None"
};

uint8_t
get_running_slot_id(void)
{
  return (shared.running_slot_id);
}

uint8_t *
get_name(void)
{
  return (shared.name);
}
