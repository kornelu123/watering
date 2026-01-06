/** * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pico/cyw43_arch.h"
#include "pico/stdio.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "sched.h"

#define POLL_TIME_S 10

#define TCP_PORT 8080
#define DEBUG_printf printf
#define BUF_SIZE 4096

typedef struct TCP_CLIENT_T_ {
  struct tcp_pcb *tcp_pcb;
  ip_addr_t remote_addr;
  uint8_t buffer[BUF_SIZE];
  int buffer_len;
  int sent_len;
  bool complete;
  int run_count;
  bool connected;
} TCP_CLIENT_T;

static err_t tcp_client_close(void *arg) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
  err_t err = ERR_OK;
  if (state->tcp_pcb != NULL) {
    tcp_arg(state->tcp_pcb, NULL);
    tcp_poll(state->tcp_pcb, NULL, 0);
    tcp_sent(state->tcp_pcb, NULL);
    tcp_recv(state->tcp_pcb, NULL);
    tcp_err(state->tcp_pcb, NULL);
    err = tcp_close(state->tcp_pcb);
    if (err != ERR_OK) {
      DEBUG_printf("close failed %d, calling abort\n", err);
      tcp_abort(state->tcp_pcb);
      err = ERR_ABRT;
    }
    state->tcp_pcb = NULL;
  }
  return err;
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
  DEBUG_printf("tcp_client_sent %u\n", len);
  state->sent_len += len;

  return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
  if (err != ERR_OK) {
    printf("connect failed %d\n", err);
    return -1;
  }
  state->connected = true;
  DEBUG_printf("Waiting for buffer from server\n");
  return ERR_OK;
}

static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
  DEBUG_printf("tcp_client_poll\n");
  return 0;
}

static void tcp_client_err(void *arg, err_t err) {
  if (err != ERR_ABRT) {
    DEBUG_printf("tcp_client_err %d\n", err);
  }
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;

  cyw43_arch_lwip_check();
  if (p->tot_len > 0) {
    DEBUG_printf("recv %d err %d\n", p->tot_len, err);

    const uint16_t buffer_left = BUF_SIZE - state->buffer_len;
    state->buffer_len += pbuf_copy_partial(p, state->buffer + state->buffer_len,
        p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
    tcp_recved(tpcb, p->tot_len);
  }
  pbuf_free(p);

  return ERR_OK;
}

static bool tcp_client_open(void *arg) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
  DEBUG_printf("Connecting to %s port %u\n", ip4addr_ntoa(&state->remote_addr), TCP_PORT);
  state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
  if (!state->tcp_pcb) {
    DEBUG_printf("failed to create pcb\n");
    return false;
  }

  tcp_arg(state->tcp_pcb, state);
  tcp_poll(state->tcp_pcb, tcp_client_poll, POLL_TIME_S * 2);
  tcp_sent(state->tcp_pcb, tcp_client_sent);
  tcp_recv(state->tcp_pcb, tcp_client_recv);
  tcp_err(state->tcp_pcb, tcp_client_err);

  state->buffer_len = 0;

  cyw43_arch_lwip_begin();
  err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, TCP_PORT, tcp_client_connected);
  cyw43_arch_lwip_end();

  return err == ERR_OK;
}

static TCP_CLIENT_T* tcp_client_init(void) {
  TCP_CLIENT_T *state = calloc(1, sizeof(TCP_CLIENT_T));
  if (!state) {
    DEBUG_printf("failed to allocate state\n");
    return NULL;
  }

  ip4addr_aton(TEST_TCP_SERVER_IP, &state->remote_addr);
  return state;
}

void __main(void) {
  TCP_CLIENT_T *state = tcp_client_init();
  if (!state) {
    return;
  }
  if (!tcp_client_open(state)) {
    return;
  }

  __run_sched();

  free(state);
}

void __attribute__((__noreturn__))main() {
  stdio_init_all();
  printf("Welcome in waterer software!\n");

  if (cyw43_arch_init()) {
    DEBUG_printf("failed to initialise\n");
    exit(-1);
  }
  cyw43_arch_enable_sta_mode();

  printf("Connecting to %s Wi-Fi...\n", WIFI_SSID);

  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    printf("failed to connect.\n");
    exit(-1);
  } else {
    printf("Connected.\n");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  }

  __main();

  cyw43_arch_deinit();
  exit(0);
}
