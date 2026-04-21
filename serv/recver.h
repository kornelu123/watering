#pragma once

#include "serv.h"
#include "ring_buf.h"

void *recv_main(void *args);

void dispatch(packet_t *packet, int client_fd, uint32_t size);

void send_packet(pico_ctx_t *pico_ctx);
void broadcast_packet(void);

// TCP responses handling

int get_running_slot_handle(packet_t *in_packet, pico_ctx_t *pico_ctx);
int get_info_handle(packet_t *in_packet, pico_ctx_t *pico_ctx);
int get_watering_ctx_handle(packet_t *in_packet, pico_ctx_t *pico_ctx);
int generic_zerolen_resp(packet_t *in_packet, pico_ctx_t *pico_ctx);

extern handle_packet dispatch_table[256];
