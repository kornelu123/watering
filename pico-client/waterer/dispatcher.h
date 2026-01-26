#pragma once

#include "proto.h"

typedef uint8_t (*handle_packet)(packet_t *in_packet, packet_t *out_packet, uint16_t *out_len);

void dispatch(packet_t *packet, uint16_t len);

void send_response(packet_t *packet);
void send_error_response(uint8_t ack, uint16_t msg_id);
void send_ok_response(uint8_t ack, uint16_t msg_id, uint16_t length);

// TCP commands
uint8_t flash_write(packet_t *in_packet, packet_t *out_packet, uint16_t *out_len);
uint8_t flash_erase(packet_t *in_packet, packet_t *out_packet, uint16_t *out_len);

uint8_t reset_handle(packet_t *in_packet, packet_t *out_packet, uint16_t *out_len);

uint8_t get_running_slot_handle(packet_t *in_packet, packet_t *out_packet, uint16_t *out_len);
uint8_t set_active_slot_handle(packet_t *in_packet, packet_t *out_packet, uint16_t *out_len);

extern handle_packet dispatch_table[256];
