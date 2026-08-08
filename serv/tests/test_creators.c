#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "controller.h"
#include "creators.h"

uint8_t slot_binary[2][SLOT_BINARY_SIZE];

int update_binary_callback(packet_t *packet, pico_ctx_t *ctx)
{
  (void)packet;
  (void)ctx;
  return 0;
}

static void check_header(const pico_ctx_t *ctx, uint8_t command,
                         uint16_t id, uint16_t length)
{
  assert(ctx->out_buf.header.cmd_ack == command);
  assert(ctx->out_buf.header.msg_id == id);
  assert(ctx->out_buf.header.length == length);
}

int main(void)
{
  pico_ctx_t ctx = {0};

  create_get_running_slot_packet(1, &ctx);
  check_header(&ctx, READ_RUNNING_SLOT_CMD, 1, 0);

  create_get_info_packet(2, &ctx);
  check_header(&ctx, GET_INFO_CMD, 2, 1);

  create_get_ctx_packet(3, &ctx);
  check_header(&ctx, GET_WATERING_CTX_CMD, 3, 1);

  create_set_active_slot_packet(1, 4, &ctx);
  check_header(&ctx, SET_ACTIVE_SLOT_CMD, 4, sizeof(set_active_slot_t));
  assert(ctx.out_buf.data.set_active_slot.slot_id == 1);

  create_set_name_packet((const uint8_t *)"fern", 5, &ctx);
  check_header(&ctx, SET_NAME_CMD, 5, 5);
  assert(memcmp(ctx.out_buf.data.set_name.name, "fern", 4) == 0);

  create_water_trigger_packet(6, &ctx, 2500);
  check_header(&ctx, TRIGGER_WATER_CMD, 6,
               1 + sizeof(trigger_water_ctx_t));
  assert(ctx.out_buf.data.trigger_water.time_ms == 2500);

  create_watering_time_packet(7, &ctx, 3000);
  check_header(&ctx, SET_WATERING_TIME, 7,
               1 + sizeof(set_watering_time_t));
  assert(ctx.out_buf.data.set_water.time_ms == 3000);

  create_water_threshold_packet(8, &ctx, 700);
  check_header(&ctx, SET_WATER_THRESHOLD, 8,
               1 + sizeof(set_watering_threshold_t));
  assert(ctx.out_buf.data.set_thresh.threshold == 700);

  create_peer_discovery_packet(9, &ctx, ROLE_DISPLAYER, 0x01020304);
  check_header(&ctx, PEER_DISCOVERY_CMD, 9, sizeof(peer_discovery_t));
  assert(ctx.out_buf.data.peer_discovery.role == ROLE_DISPLAYER);
  assert(ctx.out_buf.data.peer_discovery.ip == 0x01020304);

  return 0;
}
