#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <sys/stat.h>

#include <unistd.h>
#include <fcntl.h>

#include "proto.h"
#include "creators.h"
#include "helpers.h"
#include "dispatcher.h"

uint8_t slot_binary[2][SLOT_BINARY_SIZE];

  int
read_binary_from_file(const char *fname, uint8_t slot_id)
{
  struct stat *statbuf;
  if (stat(fname, statbuf) != 0) {
    printf("Unable to read stats from %s file \n", fname);
    return -1;
  }

  if (statbuf->st_size != SLOT_BINARY_SIZE) {
    printf("Size of file (%08X) isnt %08X\n", statbuf->st_size, SLOT_BINARY_SIZE);
    return -1;
  }

  const int fd = open(fname, O_RDONLY);

  read(fd, slot_binary[slot_id], SLOT_BINARY_SIZE);

  close(fd);
}

  int
create_erase_packet(const uint16_t part, pico_ctx_t *pico_ctx)
{
  packet_t *out_buf = &(pico_ctx->out_buf);

  out_buf->header.cmd_ack = FLASH_ERASE_CMD;
  out_buf->header.msg_id = part;
  out_buf->header.length = sizeof(erase_flash_data_t);

  write_flash_data_t *data = &(out_buf->data.flash_write);

  put_be24(SLOT_ID_TO_ADDR(pico_ctx->slot_to_update) + part*MAX_FLASH_DATA, data->addr);

  return 0;
}

  int
create_binary_packet(const uint16_t part, pico_ctx_t *pico_ctx)
{
  packet_t *out_buf = &(pico_ctx->out_buf);

  out_buf->header.cmd_ack = FLASH_WRITE_CMD;
  out_buf->header.msg_id = part;
  out_buf->header.length = sizeof(write_flash_data_t);

  write_flash_data_t *data = &(out_buf->data.flash_write);


  put_be24(SLOT_ID_TO_ADDR(pico_ctx->slot_to_update) + PART_TO_OFFSET(part), data->addr);
  memcpy((void *)data->data, &(slot_binary[pico_ctx->slot_to_update][PART_TO_OFFSET(part)]), MAX_FLASH_DATA);

  return 0;
}

  int
create_get_running_slot_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx)
{
  packet_t *out_buf = &(pico_ctx->out_buf);

  out_buf->header.cmd_ack = READ_RUNNING_SLOT_CMD;
  out_buf->header.msg_id = msg_id;
  out_buf->header.length = 0;
}

  int
update_binary_callback(packet_t *in_packet, pico_ctx_t *pico_ctx)
{

  if (PART_TO_OFFSET(in_packet->header.msg_id+1) >= SLOT_BINARY_SIZE) {
    pico_ctx->packet_callback = NULL;
    return 0;
  }

  if (in_packet->header.cmd_ack == READ_RUNNING_SLOT_CMD) {
    create_erase_packet(0, pico_ctx);
  } else if (in_packet->header.cmd_ack == FLASH_ERASE_CMD) {
    create_binary_packet(in_packet->header.msg_id, pico_ctx);
  } else if (in_packet->header.cmd_ack == FLASH_WRITE_CMD) {
    printf(" Modulo: %08X\n", PART_TO_OFFSET(in_packet->header.msg_id + 1) % FLASH_PAGE_SIZE);
    if (PART_TO_OFFSET(in_packet->header.msg_id + 1) % FLASH_PAGE_SIZE == 0) {
      create_erase_packet(in_packet->header.msg_id + 1, pico_ctx);
    } else {
      create_binary_packet(in_packet->header.msg_id + 1, pico_ctx);
    }
  }

  send_packet(pico_ctx);

  return 0;
}

  void
create_update_procedure(const uint8_t slot_id, pico_ctx_t *pico_ctx)
{
  create_get_running_slot_packet(slot_id, pico_ctx);
  pico_ctx->packet_callback = update_binary_callback;
}

  void
create_set_active_slot_packet(const uint8_t slot_id, pico_ctx_t *pico_ctx)
{
  packet_t *out_buf = &(pico_ctx->out_buf);

  out_buf->header.cmd_ack = SET_ACTIVE_SLOT_CMD;
  out_buf->header.msg_id = slot_id;
  out_buf->header.length = 1;

  out_buf->data.set_active_slot.slot_id = slot_id;
}

  void
create_reset_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx)
{
  packet_t *out_buf = &(pico_ctx->out_buf);

  out_buf->header.cmd_ack = RESET_PICO_CMD;
  out_buf->header.msg_id = msg_id;
  out_buf->header.length = 0;
}
