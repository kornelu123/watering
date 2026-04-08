#include "waterer.h"
#include <stdio.h>
#include <string.h>

#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

#define SLEEP_TIME_MS 1800000 // 30 min
#define WATERING_TIME_MS 2000 // 2s
#define PUMP_SWITCH_DELAY 500 // 0.5s

extern struct tcp_pcb *cloud_tcp;
extern struct tcp_pcb *display_tcp;

typedef enum {
    STATE_READ_SENSORS,
    STATE_WATERING_PUMP,
    STATE_STOP_PUMP
} task_state_t;

static task_state_t current_state = STATE_READ_SENSORS;

static uint8_t pumps_to_water[MAX_PUMPS];
static uint8_t pumps_count = 0;
static uint8_t current_pump_index = 0;
uint16_t current_moisture[MAX_PUMPS];

int waterer_init(void) {
    w_init();
    return 3000; 
}

// helper function
// Currently sends data as plain-text, not included in ~/common/proto.h
static void send_data(bool is_charging){
    char message[256];
    int offset = 0;

    offset += snprintf(message + offset, sizeof(message) - offset, "BAT_CHG:%d|", is_charging);
    for (uint8_t i = 0; i < MAX_PUMPS; i++) {
        offset += snprintf(message + offset, sizeof(message) - offset, "CH%d:%d|", i, current_moisture[i]);
    }

    // Send data to cloud
    if (cloud_tcp != NULL) {
        cyw43_arch_lwip_begin();

        err_t err1 = tcp_write(cloud_tcp, message, strlen(message), TCP_WRITE_FLAG_COPY);

        if (err1 == ERR_OK){
            tcp_output(cloud_tcp);
        }
        cyw43_arch_lwip_end(); 
    }
    // Send data to displayer
    if (display_tcp != NULL) {
        cyw43_arch_lwip_begin();

        err_t err2 = tcp_write(display_tcp, message, strlen(message), TCP_WRITE_FLAG_COPY);

        if(err2 == ERR_OK){
            tcp_output(display_tcp);
        }
        cyw43_arch_lwip_end(); 
    }

}

int waterer_task(void) {
    switch (current_state) {
        case STATE_READ_SENSORS: {
            pumps_count = 0;
            current_pump_index = 0;
            
            bool is_charging = w_is_battery_charging();
            
            for (uint8_t i = 0; i < MAX_PUMPS; i++) {
                current_moisture[i] = w_read_moisture(i);

                if (current_moisture[i] < MOISTURE_THRESHOLD) {
                    pumps_to_water[pumps_count] = i;
                    pumps_count++;
                }
            }

            send_data(is_charging);

            if (pumps_count > 0) {
                current_state = STATE_WATERING_PUMP;
                return 100;
            }
            else { 
                return SLEEP_TIME_MS;
            }
        }

        case STATE_WATERING_PUMP: {
            uint8_t pump_id = pumps_to_water[current_pump_index];
            
            w_pump_on(pump_id);
            
            current_state = STATE_STOP_PUMP;
            return WATERING_TIME_MS;
        }
        
        case STATE_STOP_PUMP: {
            w_pumps_off();
            
            current_pump_index++;
            
            if (current_pump_index < pumps_count) {
                current_state = STATE_WATERING_PUMP;
                return PUMP_SWITCH_DELAY;
            } else {
                current_state = STATE_READ_SENSORS;
                return SLEEP_TIME_MS;
            }
        }
    }
    // Fallback
    current_state = STATE_READ_SENSORS;
    return SLEEP_TIME_MS;
}