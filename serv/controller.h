#pragma once

#include "serv.h"
#include "creators.h"

#define STATUS_INTERVAL_S 30

extern uint8_t slot_binary[2][SLOT_BINARY_SIZE];

int update_binary_callback(packet_t *in_packet, pico_ctx_t *pico_ctx);
int end_init_callback(packet_t *in_packet, pico_ctx_t *pico_ctx);

void wakeup_controller(void);
void *main_controller(void *args);
