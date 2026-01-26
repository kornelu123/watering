#include "proto.h"

#include <stdint.h>
#include <stdio.h>

uint32_t
get_be24(uint8_t *addr_data)
{
  uint32_t addr = 0;
  addr |= addr_data[0] << 16;
  addr |= addr_data[1] << 8;
  addr |= addr_data[2];
  return addr;
}

void
put_be24(uint32_t addr, uint8_t *addr_data)
{
  addr_data[2] = addr & 0xFF;
  addr_data[1] = (addr & 0xFF00) >> 8;
  addr_data[0] = (addr & 0xFF0000) >> 16;
}

#ifdef PICO_FLASH_SIZE
int
is_flash_erased(uint32_t *addr, uint32_t size)
{
  uint32_t *ptr = addr;

  for(int i=addr; i<addr + size; i+=4) {
    if (ptr[i] != 0xFFFFFFFF) {
      return -1;
    }
  }

  return 0;
}
#endif
