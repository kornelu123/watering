#include "tcp_client.h"

#include <stdlib.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"

#include "dispatcher.h"
#include "proto.h"
#include "shared_mem.h"
#include "pico/time.h"

#define POLL_TIME_S 10
#define TCP_PORT 8080

typedef struct TCP_CLIENT_T_ {
  struct tcp_pcb *tcp_pcb;
  ip_addr_t remote_addr;
  uint8_t buffer[BUF_LEN];
  int buffer_len;
  int sent_len;
  bool complete;
  int run_count;
  bool connected;
  struct tcp_pcb **out_tcp_ptr;
} TCP_CLIENT_T;

struct tcp_pcb *cloud_tcp = NULL;
struct tcp_pcb *display_tcp = NULL;

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
  if (state != NULL) {
      free(state);
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

  if (err != ERR_OK) {
    return -1;
  }

  if (state->out_tcp_ptr != NULL) {
      *(state->out_tcp_ptr) = state->tcp_pcb; 
  }
  state->connected = true;
  return ERR_OK;
}

static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
  return 0;
}

static void tcp_client_err(void *arg, err_t err) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
  if (state) {
    state->tcp_pcb = NULL; 
    state->connected = false;
  }
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
  TCP_CLIENT_T *state = (TCP_CLIENT_T*) arg;

  cyw43_arch_lwip_check();
  if (!p) {
    return err;
  }

  if (p->tot_len > 0) {
    state->buffer_len = pbuf_copy_partial(p, state->buffer, p->tot_len , 0);

    // Passes the active PCB to the dispatcher to route the ACK correctly.
    dispatch((packet_t *)state->buffer, state->buffer_len, tpcb);
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

static TCP_CLIENT_T* tcp_client_init(const char* ip_addr, struct tcp_pcb **out_ptr) {
  TCP_CLIENT_T *state = calloc(1, sizeof(TCP_CLIENT_T));
  if (!state) {
    return NULL;
  }

  ip4addr_aton(ip_addr, &state->remote_addr);
  state->out_tcp_ptr = out_ptr;
  return state;
}

bool tcp_client_init_and_connect(const char* ip_addr, struct tcp_pcb **out_ptr) {
  TCP_CLIENT_T *state = tcp_client_init(ip_addr, out_ptr);
  if (!state) {
    return false;
  }

  uint32_t last_try_time = 0;

  while (!state->connected) {
    if (state->tcp_pcb == NULL) {
      uint32_t current_time = time_us_32();

      if (current_time - last_try_time > 100000) {
            tcp_client_open(state);
            last_try_time = time_us_32();
        }
    }
    __wfe(); 
  }

  return true;
}