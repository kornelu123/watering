#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "lwip/tcp.h"
#include <stdbool.h>

extern struct tcp_pcb *cloud_tcp;
extern struct tcp_pcb *display_tcp;

bool tcp_client_init_and_connect(const char* ip_addr, struct tcp_pcb **out_ptr);

#endif