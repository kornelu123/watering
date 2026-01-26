#pragma once

#include "proto.h"
#include "dispatcher.h"

#define SLOT_ID_TO_ADDR(x)          (0x020000 + 0x080000*x)
#define PART_TO_OFFSET(x)           (MAX_FLASH_DATA*(x))

#define SLOT_BINARY_SIZE            512*1024

extern uint8_t slot_binary[2][SLOT_BINARY_SIZE];
extern uint8_t buf[BUF_LEN];

extern packet_t *out_buf;

int read_binary_from_file(const char *fname, uint8_t slot_id);

int create_erase_packet(const uint16_t part, pico_ctx_t *pico_ctx);
int create_binary_packet(const uint16_t part, pico_ctx_t *pico_ctx);
void create_update_procedure(const uint8_t slot_id, pico_ctx_t *pico_ctx);

int create_get_running_slot_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx);
void create_set_active_slot_packet(const uint8_t slot_id, pico_ctx_t *pico_ctx);

void create_reset_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx);
