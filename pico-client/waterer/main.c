/** * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "pico/cyw43_arch.h"
#include "pico/stdio.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "dispatcher.h"
#include "sched.h"
#include "proto.h"
#include "shared_mem.h"

#define POLL_TIME_S 10

#define TCP_PORT 8080

#if !defined(PROJECT_VERSION_MAJOR) || !defined(PROJECT_VERSION_MINOR)|| !defined(PROJECT_VERSION_PATCH)
  #error "PROJECT_VERSION cannot be read"
#endif

typedef struct TCP_CLIENT_T_ {
  struct tcp_pcb *tcp_pcb;
  ip_addr_t remote_addr;
  uint8_t buffer[BUF_LEN];
  int buffer_len;
  int sent_len;
  bool complete;
  int run_count;
  bool connected;
} TCP_CLIENT_T;

struct tcp_pcb *main_tcp;

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
      tcp_abort(state->tcp_pcb);
      err = ERR_ABRT;
    }
    state->tcp_pcb = NULL;
  }
  return err;
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
  state->sent_len += len;

  return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
  main_tcp = state->tcp_pcb;

  if (err != ERR_OK) {
    return -1;
  }
  state->connected = true;
  return ERR_OK;
}

static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
  return 0;
}

static void tcp_client_err(void *arg, err_t err) {
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;

  cyw43_arch_lwip_check();
  if (!p) {
    return err;
  }

  if (p->tot_len > 0) {
    const uint16_t buffer_left = BUF_LEN;
    state->buffer_len = pbuf_copy_partial(p, state->buffer,
        p->tot_len , 0);

    dispatch((packet_t *)state->buffer, state->buffer_len);
  }
  if (p) {
    pbuf_free(p);
  }
  state->buffer_len = 0;

  tcp_recved(tpcb, p->tot_len);

  return ERR_OK;
}

static bool tcp_client_open(void *arg) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
  state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
  if (!state->tcp_pcb) {
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

  while(true) {
    if (tcp_client_open(state)) {
      break;
    }
  }

  __run_sched();

  free(state);
}

void __attribute__((__noreturn__))main() {
  stdio_init_all();

  if (cyw43_arch_init()) {
    exit(-1);
  }

  cyw43_arch_enable_sta_mode();

  while (1) {
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    } else {
      printf("Connected.\n");
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
      break;
    }
  }

  printf("Welcome in waterer software!\n");
  printf("Running at %08X\n", (uint32_t)CUR_SLOT_ORIGIN);
  printf("Running slot  in flash: %02X\n", get_running_slot_id());
  printf("Connecting to %s Wi-Fi...\n", WIFI_SSID);

  __main();

  cyw43_arch_deinit();
  exit(0);
}
