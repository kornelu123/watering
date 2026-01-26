#pragma once

#include "proto.h"

#include <pthread.h>

#include <stdint.h>

typedef struct pico_ctx pico_ctx_t;

struct pico_ctx {
  int fd;

  uint8_t slot_to_update;
  uint8_t cur_slot_id;
  uint16_t last_msg_id;

  pthread_mutex_t pico_mut;
  packet_t out_buf;

  int (*packet_callback)(packet_t *in_packet, pico_ctx_t *ctx);
};


extern uint32_t pico_count;
extern pico_ctx_t *pico_ctxs;

typedef int (*handle_packet)(packet_t *in_packet, pico_ctx_t *pico_ctx);

int del_from_ctxs(const int fd);
void add_to_ctxs(const int fd);
void dispatch(packet_t *packet, int client_fd, uint32_t size);

void send_packet(pico_ctx_t *pico_ctx);
void broadcast_packet(void);

// TCP responses handling

int get_running_slot_handle(packet_t *in_packet, pico_ctx_t *pico_ctx);
int set_active_slot_handle(packet_t *in_packet, pico_ctx_t *pico_ctx);

int flash_write(packet_t *in_packet, pico_ctx_t *pico_ctx);
int flash_erase(packet_t *in_packet, pico_ctx_t *pico_ctx);

extern handle_packet dispatch_table[256];
