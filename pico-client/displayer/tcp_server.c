#include "tcp_server.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "proto.h"

struct tcp_pcb *display_tcp = NULL;
packet_t rx_packet = {0};
volatile bool raw_packet_ready = false;

static void tcp_server_err(void *arg, err_t err) {
    display_tcp = NULL;
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        display_tcp = NULL;
        return ERR_OK;
    }

    uint16_t len = (p->len < sizeof(packet_t)) ? p->len : sizeof(packet_t);
    memcpy(&rx_packet, p->payload, len);
    
    raw_packet_ready = true;

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || newpcb == NULL) {
        return ERR_VAL;
    }

    display_tcp = newpcb;
    tcp_err(newpcb, tcp_server_err);
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

