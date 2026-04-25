#include "proto.h"
#include "tcp_server.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

#define WAIT_TIME 10000

volatile bool new_data;
uint16_t current_moisture[MAX_PUMPS];
bool pump_active[MAX_PUMPS];
uint8_t battery_level;
uint8_t water_level;

extern struct tcp_pcb *display_tcp;

extern packet_t rx_packet;
extern volatile bool raw_packet_ready;

static uint16_t msg_id_counter = 0;

int comm_task(void) {
    if (raw_packet_ready) {
        raw_packet_ready = false;

        if (rx_packet.header.cmd_ack == GET_WATERING_CTX_CMD) {
            get_watering_ctx_t *ctx = &rx_packet.data.get_ctx;
            
            battery_level = ctx->battery_lvl;
            water_level = ctx->water_lvl;

            for (int i = 0; i < MAX_PUMPS; i++) {
                current_moisture[i] = ctx->moisture_lvl[i];
                pump_active[i] = (ctx->active_pumps_mask & (1 << i)) != 0;
            }

            new_data = true;
        }
        return WAIT_TIME;
    }

    packet_t tx_packet = {0};
    tx_packet.header.cmd_ack = GET_WATERING_CTX_CMD;
    tx_packet.header.msg_id = ++msg_id_counter;
    tx_packet.header.length = 0;

    if (display_tcp != NULL) {
        cyw43_arch_lwip_begin();
        tcp_write(display_tcp, &tx_packet, HEADER_LEN, TCP_WRITE_FLAG_COPY);
        tcp_output(display_tcp);
        cyw43_arch_lwip_end();
    }

    return 500;
}