#pragma once

#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

#include "proto.h"
#include "ring_buf.h"

#define PORT "8080"

typedef struct pico_ctx pico_ctx_t;

typedef enum {
  INIT = 0,
  RUNNING = 1,
  SET_NAME = 2,
  OTA = 3,
} pico_messages_state_t;

struct pico_ctx {
  int fd;

  uint8_t slot_to_update;
  uint8_t cur_slot_id;
  uint8_t active_slot_id;

  uint16_t last_msg_id;
  uint64_t pico_id;

  uint8_t pico_name[MAX_NAME_LEN];

  bool action_completed;
  time_t status_deadline;
  pico_messages_state_t intern_state;

  pthread_mutex_t pico_mut;
  packet_t out_buf;

  get_watering_ctx_t watering_ctx;

  uint8_t recv_buf[MAX_DATA_LEN];
  size_t  recv_len;

  int (*packet_callback)(packet_t *in_packet, pico_ctx_t *ctx);
};

extern volatile uint32_t pico_count;
extern pico_ctx_t *pico_ctxs;

typedef int (*handle_packet)(packet_t *in_packet, pico_ctx_t *pico_ctx);

int del_from_ctxs(const int fd);
void add_to_ctxs(const int fd);

typedef struct intern_packet_t {
  int fd;
  uint32_t size;
  packet_t pack;
} intern_packet_t;

DECLARE_RINGBUF_W_STRUCT(packet, intern_packet_t);

extern pthread_cond_t packet_buf_cond;
extern pthread_mutex_t packet_buf_mutex;

extern pthread_cond_t control_buf_cond;
extern pthread_mutex_t control_buf_mutex;

void *serv_main(void *ptr);
void broadcast_packet(void);
void send_packet(pico_ctx_t *pico_ctx);
