#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "creators.h"
#include "serv.h"
#include "controller.h"
#include "logger.h"
#include "proto.h"
#include "helpers.h"
#include "cloud.h"

#define PART_TO_OFFSET(x)           (MAX_FLASH_DATA*(x))

uint8_t slot_binary[2][SLOT_BINARY_SIZE];

int
end_init_callback(packet_t *in_packet, pico_ctx_t *pico_ctx)
{
  pico_ctx->intern_state = RUNNING;
  pico_ctx->packet_callback = NULL;

  create_get_ctx_packet(0xDD, pico_ctx);
  send_packet(pico_ctx);

  wakeup_controller();

  return 0;
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
wakeup_controller(void)
{
  log_info("Waking up controller\n");
  pthread_cond_signal(&control_buf_cond);
}

void *
main_controller(void *args)
{
  init_cloud_conn(); 

  struct timespec now;

  while (1) {
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
      log_crit("clock_gettime failed\n");
      exit(-1);
    }

    uint32_t min_deadline = now.tv_sec + 10;

    for (uint32_t i = 0; i < pico_count; i++) {
      const pico_messages_state_t state = pico_ctxs[i].intern_state;
      const time_t state_deadline = pico_ctxs[i].status_deadline;
      switch (state) {
        case INIT:
          {
            create_get_info_packet(0x00, &pico_ctxs[i]);
            pico_ctxs[i].packet_callback = end_init_callback;
            send_packet(&pico_ctxs[i]);

            pico_ctxs[i].status_deadline = now.tv_sec + 5;

            if (state_deadline < min_deadline) {
              min_deadline = pico_ctxs[i].status_deadline;
            }

            break;
          }
        case RUNNING:
          {
            if (state_deadline <= now.tv_sec) {
              create_get_ctx_packet(0xDD, &pico_ctxs[i]);
              send_packet(&pico_ctxs[i]);

              if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
                log_crit("clock_gettime failed\n");
                exit(-1);
              }

              uint32_t min_deadline = now.tv_sec + 10;

              pthread_mutex_lock(&(pico_ctxs[i].pico_mut));

              pico_ctxs[i].status_deadline;

              pthread_mutex_unlock(&(pico_ctxs[i].pico_mut));
      
              if (cloud_post_telemetry(pico_ctxs[i].pico_name, pico_ctxs[i].watering_ctx.moisture_lvl,
                    pico_ctxs[i].watering_ctx.water_lvl, pico_ctxs[i].watering_ctx.battery_lvl, pico_ctxs[i].watering_ctx.uptime) ) {
                log_err("Failed to post telemetry\n");
              }
            }

            if (state_deadline < min_deadline) {
              min_deadline = pico_ctxs[i].status_deadline;
            }
            break;
          }
        case SET_NAME:
          {
            log_info("We're requested to set name\n");
            break;
          }
        case OTA:
          {
            log_info("We're requested to perform OTA\n");
            break;
          }
        default:
          log_crit("Controller is in a state that he shouldn't be in: %u\n",
                   pico_ctxs[i].intern_state);
          exit(-1);
      }
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5;

    pthread_mutex_lock(&control_buf_mutex);
    pthread_cond_timedwait(&control_buf_cond, &control_buf_mutex, &ts);
    pthread_mutex_unlock(&control_buf_mutex);
  }

  deinit_cloud_conn(); 
}
