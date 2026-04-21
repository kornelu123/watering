#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <sys/stat.h>

#include <unistd.h>
#include <fcntl.h>

#include "proto.h"
#include "creators.h"
#include "helpers.h"
#include "serv.h"
#include "logger.h"
#include "controller.h"

  void
create_update_procedure(const uint8_t slot_id, pico_ctx_t *pico_ctx)
{
  create_get_running_slot_packet(slot_id, pico_ctx);
  pico_ctx->packet_callback = update_binary_callback;
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
create_get_running_slot_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx)
{
  packet_t *out_buf = &(pico_ctx->out_buf);

  out_buf->header.cmd_ack = READ_RUNNING_SLOT_CMD;
  out_buf->header.msg_id = msg_id;
  out_buf->header.length = 0;
}

  void
create_set_active_slot_packet(const uint8_t slot_id, const uint16_t msg_id, pico_ctx_t *pico_ctx)
{
  packet_t *out_buf = &(pico_ctx->out_buf);

  out_buf->header.cmd_ack = SET_ACTIVE_SLOT_CMD;
  out_buf->header.msg_id = msg_id;
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

  void
create_set_name_packet(const uint8_t *name, const uint16_t msg_id, pico_ctx_t *pico_ctx)
{
  packet_t *out_buf = &(pico_ctx->out_buf);

  out_buf->header.cmd_ack = SET_NAME_CMD;
  out_buf->header.msg_id = msg_id;

  strncpy(out_buf->data.set_name.name, name, MAX_NAME_LEN);

  int len = strlen(name);
  if (len > MAX_NAME_LEN) {
    len = MAX_NAME_LEN;
  }

  out_buf->header.length = 1 + len;
}

  void
create_get_info_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx)
{
  packet_t *out_buf = &(pico_ctx->out_buf);

  out_buf->header.cmd_ack = GET_INFO_CMD;
  out_buf->header.msg_id = msg_id;
  out_buf->header.length = 1;
}

  void
create_get_ctx_packet(const uint16_t msg_id, pico_ctx_t *pico_ctx)
{
  packet_t *out_buf = &(pico_ctx->out_buf);

  out_buf->header.cmd_ack = GET_WATERING_CTX_CMD;
  out_buf->header.msg_id = msg_id;
  out_buf->header.length = 1;
}
