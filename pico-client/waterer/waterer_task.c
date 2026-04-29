#include <stdio.h>
#include <string.h>
#include "proto.h"
#include "tcp_client.h"
#include "waterer.h"

#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

#define WATERING_TIME_MS 2000 // 2s
#define PUMP_SWITCH_DELAY 500 // 0.5s
#define MOISTURE_THRESHOLD  512 // 0-1023

extern struct tcp_pcb *cloud_tcp;
extern struct tcp_pcb *display_tcp;

typedef enum {
    STATE_READ_SENSORS,
    STATE_SAMPLE_SENSORS,
    STATE_WATERING_START_12V,
    STATE_WATERING_OPEN_VALVE,
    STATE_STOP_PUMP
} task_state_t;

static task_state_t current_state = STATE_READ_SENSORS;

static uint8_t pumps_to_water[MAX_PUMPS];
static uint8_t pumps_count = 0;
static uint8_t current_pump_index = 0;
uint16_t current_moisture[MAX_PUMPS];

static uint32_t moisture_accumulator[MAX_PUMPS];
static uint8_t sample_count = 0;
static uint8_t sample_limit = 10;

uint8_t cached_battery_lvl = 0;
uint8_t cached_water_lvl = 0;

static uint32_t sleep_time_ms = 1800000;

void w_set_sleep_time(uint32_t new_time_ms) {
        sleep_time_ms = new_time_ms;
}

int waterer_init(void) {
    w_init();
    return 3000; 
}

void w_get_watering_stats(get_watering_ctx_t *ctx) {
    if (ctx == NULL) return;

    ctx->battery_lvl = cached_battery_lvl;
    ctx->water_lvl = cached_water_lvl;
    ctx->active_pumps_mask = w_get_active_pumps_mask();
    ctx->uptime = to_ms_since_boot(get_absolute_time()) / 1000;

    for (int i = 0; i < MAX_PUMPS; i++) {
        if (ctx->active_pumps_mask & (1 << i)) {
            ctx->moisture_lvl[i] = current_moisture[i];
        } else {
            ctx->moisture_lvl[i] = 0;
        }
    }
}

static void send_data(struct tcp_pcb *pcb){
    if (pcb == NULL){
        return;
    }

    packet_t tx_packet = {0};
    tx_packet.header.cmd_ack = GET_WATERING_CTX_CMD;
    tx_packet.header.length = sizeof(get_watering_ctx_t);
    
    w_get_watering_stats(&tx_packet.data.get_ctx);

    tcp_write(pcb, &tx_packet, sizeof(header_t) + tx_packet.header.length, TCP_WRITE_FLAG_COPY);
    tcp_output(pcb);
}

int waterer_task(void) {
    switch (current_state) {
        case STATE_READ_SENSORS: {
            pumps_count = 0;
            current_pump_index = 0;
            sample_count = 0;
            memset(moisture_accumulator, 0, sizeof(moisture_accumulator));


            cached_battery_lvl = w_read_battery();
            cached_water_lvl = w_read_water_level();
            current_state = STATE_SAMPLE_SENSORS;
            return 2;
        }
        case STATE_SAMPLE_SENSORS: {
            uint8_t active_mask = w_get_active_pumps_mask();
            
            for (uint8_t i = 0; i < MAX_PUMPS; i++) {
                if (active_mask & (1 << i)) {
                    moisture_accumulator[i] += w_read_moisture_raw(i);
                }
            }
            
            sample_count++;
            
            if (sample_count < sample_limit) {
                return 2;
            }
            
            for (uint8_t i = 0; i < MAX_PUMPS; i++) {
                if (active_mask & (1 << i)) {
                    current_moisture[i] = (uint16_t)(moisture_accumulator[i] / sample_limit);

                    if (current_moisture[i] < MOISTURE_THRESHOLD) {
                        pumps_to_water[pumps_count] = i;
                        pumps_count++;
                    }
                } else {
                    current_moisture[i] = 0;
                }
            }

            send_data(cloud_tcp);

            if (pumps_count > 0) {
                current_state = STATE_WATERING_START_12V;
                return 100;
            } else { 
                current_state = STATE_READ_SENSORS;
                return sleep_time_ms;
            }

        }
            
        case STATE_WATERING_START_12V: {
            w_pump_enable_12v();
            
            current_state = STATE_WATERING_OPEN_VALVE;
            return 20;
        }

        case STATE_WATERING_OPEN_VALVE: {
            uint8_t pump_id = pumps_to_water[current_pump_index];
            w_pump_select(pump_id);
            
            current_state = STATE_STOP_PUMP;
            return WATERING_TIME_MS;
        }
        
        case STATE_STOP_PUMP: {
            w_pumps_off();
            
            current_pump_index++;
            
            if (current_pump_index < pumps_count) {
                current_state = STATE_WATERING_START_12V;
                return PUMP_SWITCH_DELAY;
            } else {
                current_state = STATE_READ_SENSORS;
                return sleep_time_ms;
            }
        }
    }
    // Fallback
    current_state = STATE_READ_SENSORS;
    return sleep_time_ms;
}