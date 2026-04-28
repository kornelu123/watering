#pragma once

#include "proto.h"
#include "serv.h"

#define SLOT_BINARY_SIZE            (SLOT1_ORIGIN - SLOT0_ORIGIN)

#define SLOT_ID_TO_ADDR(x)          (SLOT0_ORIGIN + (SLOT_BINARY_SIZE)*x)
#define PART_TO_OFFSET(x)           (MAX_FLASH_DATA*(x))

extern uint8_t buf[BUF_LEN];
extern packet_t *out_buf;

int read_binary_from_file(const char *fname, uint8_t slot_id);

int create_erase_packet(const uint16_t part, pico_ctx_t *pico_ctx);
int create_binary_packet(const uint16_t part, pico_ctx_t *pico_ctx);
void create_update_procedure(const uint8_t slot_id, pico_ctx_t *pico_ctx);

int create_get_running_slot_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx);
void create_set_active_slot_packet(const uint8_t slot_id, const uint16_t msg_id, pico_ctx_t *pico_ctx);

void create_set_name_packet(const uint8_t *name, const uint16_t msg_id, pico_ctx_t *pico_ctx);
void create_get_info_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx);

void create_reset_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx);

void create_get_ctx_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx);

