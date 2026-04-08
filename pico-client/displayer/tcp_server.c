#include "tcp_server.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

volatile bool new_data = false;
uint16_t current_moisture[MAX_PUMPS] = {0};
bool is_charging = false;

// Callback: Receiving the data
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char buffer[256];
    uint16_t len = (p->len < sizeof(buffer) - 1) ? p->len : sizeof(buffer) - 1;
    memcpy(buffer, p->payload, len);
    buffer[len] = '\0';

    // Parsing the data

    int chg_status;
    if (sscanf(buffer, "BAT_CHG:%d|", &chg_status) == 1) {
        is_charging = (chg_status != 0);
    }

    for (int i = 0; i < MAX_PUMPS; i++) {
        char search_tag[10];
        snprintf(search_tag, sizeof(search_tag), "CH%d:", i);
        
        char *ptr = strstr(buffer, search_tag);
        if (ptr != NULL) {
            current_moisture[i] = (uint16_t)atoi(ptr + strlen(search_tag));
        }
    }

    new_data = true;

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

// Callback: Connection attempt
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_server_recv);
    
    return ERR_OK;
}

void tcp_server_init(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        return;
    }

    cyw43_arch_lwip_begin();
    err_t err = tcp_bind(pcb, IP_ADDR_ANY, LISTEN_PORT);
    
    if (err == ERR_OK) {
        pcb = tcp_listen(pcb);
        tcp_accept(pcb, tcp_server_accept);
    }
    cyw43_arch_lwip_end();
}

