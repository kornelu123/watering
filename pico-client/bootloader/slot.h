#pragma once

#define CRC_SIZE sizeof(uint32_t)
#define SLOT_SIZE (512*1024 - CRC_SIZE)

typedef struct watering_slot_t {
  uint8_t data[SLOT_SIZE];
  uint32_t crc;
} watering_slot_t;

watering_slot_t *slots[3] = {
  (watering_slot_t *)SLOT0_ORIGIN,
  (watering_slot_t *)SLOT1_ORIGIN,
  (watering_slot_t *)SLOT2_ORIGIN
};
