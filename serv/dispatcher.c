#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <sys/socket.h>

#include "dispatcher.h"

pico_ctx_t *pico_ctxs;
uint32_t pico_count = 0;

handle_packet dispatch_table[256] = {
  [READ_RUNNING_SLOT_CMD] = get_running_slot_handle,
  [SET_ACTIVE_SLOT_CMD] = flash_erase,
  [FLASH_WRITE_CMD] = flash_write,
  [FLASH_ERASE_CMD] = flash_erase,
};

  void
add_to_ctxs(const int fd)
{
  pico_ctxs = (pico_ctx_t *)realloc(pico_ctxs, sizeof(pico_ctx_t)*(pico_count + 1));

  pico_ctxs[pico_count].fd = fd;
  pico_ctxs[pico_count].last_msg_id = 0;
  pico_ctxs[pico_count].cur_slot_id = 0xFF;

  pthread_mutex_init(&(pico_ctxs[pico_count].pico_mut), NULL);

  pico_count++;
}

  int
del_from_ctxs(const int fd)
{
  int i;

  for (i=0; i<pico_count; i++) {
    if (pico_ctxs[i].fd == fd) {
      break;
    }
  }

  if (i == pico_count)
  {
    return -1;
  }

  if (i != pico_count -1) {
    memcpy((void *)&pico_ctxs[i], (void *)&pico_ctxs[pico_count-1], sizeof(pico_ctx_t));
  }

  pico_count--;
  pico_ctxs = realloc(pico_ctxs, sizeof(pico_ctx_t)*pico_count);

  return 0;
}

  void
dispatch(packet_t *packet, int client_fd, uint32_t size)
{
  int i;

  for (i=0; i<pico_count; i++) {
    if (pico_ctxs[i].fd == client_fd) {
      break;
    }
  }

  if (i == pico_count) {
    printf("Can't find pico fd in table\n");
    return;
  }

  if (pico_ctxs[i].last_msg_id != packet->header.msg_id) {
    printf("Got unexpected message id\n");
    return;
  }

  if (packet->header.length != 0) {
    if (packet->header.length != size - sizeof(header_t)) {
      printf("Size is equal to : %d, expected : %d\n",
              packet->header.length, size - sizeof(header_t));
      return;
    }
  } else {
    if (packet->header.length != size - sizeof(header_t)) {
      printf("Length error\n");
      return;
    }
  }

  const uint8_t cmd_ack = packet->header.cmd_ack;

  if (IS_ERR_ACK(cmd_ack)) {
    printf("Got error header: %02X\n", packet->header.cmd_ack);
    return;
  }

  if (dispatch_table[cmd_ack] == NULL) {
    printf("Got unknown ack value\n");
    return;
  }

  if (dispatch_table[cmd_ack](packet, &pico_ctxs[i])) {
    printf("Error during dispatch of message\n");
    return;
  }

  if (pico_ctxs[i].packet_callback == NULL) {
    printf("Packet callback is none");
    return;
  }

  if (pico_ctxs[i].packet_callback(packet, &pico_ctxs[i])) {
    printf("Error during callback of message\n");
    return;
  }
}

  void
broadcast_packet(void)
{
  for (int i=0; i<pico_count; i++)
  {
    send_packet(&(pico_ctxs[i])); 
  }
}

  void
send_packet(pico_ctx_t *pico_ctx)
{
  packet_t *out_packet = &(pico_ctx->out_buf);
  pico_ctx->last_msg_id = out_packet->header.msg_id;

  printf("Sending: \n");
  for (int i=0; i<out_packet->header.length + sizeof(header_t); i++) {
    printf("%02X:", ((uint8_t *)out_packet)[i]);
    if (i % 16 == 15) {
      printf("\n");
    }
  }
  send(pico_ctx->fd, (const void *)out_packet,
      out_packet->header.length + sizeof(header_t),
      MSG_CONFIRM);
}


  int
get_running_slot_handle(packet_t *in_packet, pico_ctx_t *pico_ctx) 
{
  if (in_packet->header.length != sizeof(read_running_slot_resp_t)) {
    printf("Length is not the length of valid response\n");
    return -1;
  }

  const read_running_slot_resp_t *resp = &(in_packet->data.read_running_slot);

  pico_ctx->cur_slot_id = resp->slot_id;
  pico_ctx->slot_to_update = resp->slot_id == 0 ? 1 : 0;

  return 0;
}

  int
flash_erase(packet_t *in_packet, pico_ctx_t *pico_ctx) 
{
  if (in_packet->header.length != 0) {
    printf("Length is not the length of valid response\n");
    return -1;
  }

  return 0;
}

  int
flash_write(packet_t *in_packet, pico_ctx_t *pico_ctx) 
{
  if (in_packet->header.length != 0) {
    printf("Length is not the length of valid response\n");
    return -1;
  }

  return 0;
}

  int
set_active_slot_handle(packet_t *in_packet, pico_ctx_t *pico_ctx)
{
  if (in_packet->header.length != 0) {
    printf("Length is not the length of valid response\n");
    return -1;
  }

  return 0;
}
