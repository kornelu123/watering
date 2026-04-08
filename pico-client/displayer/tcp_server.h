#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_PUMPS 8
#define LISTEN_PORT 8080

extern volatile bool new_data;
extern uint16_t current_moisture[MAX_PUMPS];
extern bool is_charging;

void tcp_server_init(void);

#endif