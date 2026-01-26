#pragma once

#define FLASH_PAGE_SIZE 4096

#include <stdint.h>

uint32_t get_be24(uint8_t *addr_data);
void put_be24(uint32_t addr, uint8_t *addr_data);
int is_flash_erased(uint32_t addr, uint32_t size);
