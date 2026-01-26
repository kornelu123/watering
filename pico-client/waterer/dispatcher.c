#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "shared_mem.h"

#include "lwip/tcp.h"

#include "dispatcher.h"

#include "reset.h"

#define SLOT_SIZE (SLOT1_ORIGIN - SLOT0_ORIGIN)

#define dbg_printf printf

extern struct tcp_pcb *main_tcp;

uint8_t tx_buf[sizeof(packet_t)];

handle_packet dispatch_table[256] = {
  [READ_RUNNING_SLOT_CMD] = get_running_slot_handle,
  [SET_ACTIVE_SLOT_CMD] = set_active_slot_handle,
  [RESET_PICO_CMD] = reset_handle,
  [FLASH_WRITE_CMD] = flash_write,
  [FLASH_ERASE_CMD] = flash_erase,
};

static inline int
set_running_slot(uint8_t slot_id)
{
  shared_mem_t cpy;
  memcpy(&cpy, _shared, sizeof(shared_mem_t));

  if (slot_id > 1) {
    return -1;
  }

  uint32_t ints = save_and_disable_interrupts();

  flash_range_erase((uint32_t)_shared, FLASH_PAGE_SIZE);

  cpy.running_slot_id = slot_id;
  memcpy(_shared, &cpy, sizeof(shared_mem_t));

  flash_range_program((uint32_t) _shared, (const uint8_t *)&cpy, FLASH_PAGE_SIZE);

  restore_interrupts(ints);

  return 0;
}

void
dispatch(packet_t *in_packet, uint16_t len)
{
  const uint8_t cmd = in_packet->header.cmd_ack;
  const uint16_t msg_id = in_packet->header.msg_id;

  //dbg_printf("Recved:\n");
  //for (int i=0; i<len; i++) {
  //  dbg_printf("%02X:", ((uint8_t *)in_packet)[i]);
  //  if ( i%8 == 7) {
  //    dbg_printf("\n");
  //  }
  //}
  //dbg_printf("\n\n");

  fflush(stdout);

  if (dispatch_table[cmd] == NULL) {
    send_error_response(ACK_CMD_ERR, msg_id);
    printf("Not known command: %2X\n", cmd);
    return;
  }

  if (len != sizeof(header_t) + in_packet->header.length) {
    send_error_response(ACK_LEN_ERR, msg_id);
    printf("Packet length: %u, exp: %u\n", len, in_packet->header.length + sizeof(header_t));
    return;
  }

  uint16_t resp_len = 0;
  uint8_t ack = dispatch_table[cmd](in_packet, (packet_t *)tx_buf, &resp_len);

  if (ack == ACK_OK) {
    ack = cmd;
  }

  send_ok_response(ack, msg_id, resp_len);
}

void
send_response(packet_t *packet)
{
  tcp_write(main_tcp, packet, (sizeof(header_t) + packet->header.length), TCP_WRITE_FLAG_MORE);
}

void
send_error_response(uint8_t ack, uint16_t msg_id)
{
  packet_t *packet = (packet_t *)tx_buf;

  packet->header.cmd_ack = ack;
  packet->header.msg_id = msg_id;
  packet->header.length = 0;

  send_response(packet);
}

void
send_ok_response(uint8_t ack, uint16_t msg_id, uint16_t length)
{
  packet_t *packet = (packet_t *)tx_buf;

  packet->header.cmd_ack = ack;
  packet->header.msg_id = msg_id;
  packet->header.length = length;

  send_response(packet);
}

// TCP commands

uint8_t
flash_erase(packet_t *packet, packet_t *out_packet, uint16_t *out_len)
{
  const uint32_t addr = get_be24(packet->data.flash_erase.addr);

  if ( packet->header.length != sizeof(erase_flash_data_t) ) {
    dbg_printf("header length = %x, exp = %x\n", packet->header.length, sizeof(write_flash_data_t));
    return ACK_LEN_ERR;
  }

  if ( addr + FLASH_PAGE_SIZE > CUR_SLOT_ORIGIN && addr < CUR_SLOT_ORIGIN + SLOT_SIZE ) {
    dbg_printf("Addr invalid: %x, cur slot: %x/%x\n", addr
                                                    , CUR_SLOT_ORIGIN, CUR_SLOT_ORIGIN + SLOT_SIZE);
    return ACK_PARAM_ERR;
  }

  if ( addr >= BOOTLOADER_ORIGIN && addr < SLOT0_ORIGIN ) {
    dbg_printf("Addr invalid: %x, reserved addresses: %x/%x\n", addr
                                                              , BOOTLOADER_ORIGIN, SLOT0_ORIGIN);
    return ACK_PARAM_ERR;
  }

  if ( addr % FLASH_PAGE_SIZE != 0) {
    dbg_printf("Addr invalid: %x", addr);
    return ACK_PARAM_ERR;
  }
  uint32_t ints = save_and_disable_interrupts();

  flash_range_erase(addr, FLASH_PAGE_SIZE);
  
  restore_interrupts(ints);

  *out_len = 0;

  return ACK_OK;
}

uint8_t
flash_write(packet_t *packet, packet_t *out_packet, uint16_t *out_len)
{
  const uint32_t addr = get_be24(packet->data.flash_write.addr);

  if ( packet->header.length != sizeof(write_flash_data_t) ) {
    dbg_printf("header length = %x, exp = %x\n", packet->header.length, sizeof(write_flash_data_t));
    return ACK_LEN_ERR;
  }

  dbg_printf("Header: %02X\n"
             "Msg Id: %04X\n"
             "Length: %04X\n"
             "Addr:   %08X\n",
             packet->header.cmd_ack,
             packet->header.msg_id,
             packet->header.length,
             addr);

  if ( addr + MAX_FLASH_DATA >= CUR_SLOT_ORIGIN && addr < CUR_SLOT_ORIGIN + SLOT_SIZE ) {
    dbg_printf("Addr invalid: %x, cur slot: %x/%x\n", addr
                                                    , CUR_SLOT_ORIGIN, CUR_SLOT_ORIGIN + SLOT_SIZE);
    return ACK_PARAM_ERR;
  }

  if ( addr >= BOOTLOADER_ORIGIN && addr < SLOT0_ORIGIN ) {
    dbg_printf("Addr invalid: %x, reserved addresses: %x/%x\n", addr
                                                              , BOOTLOADER_ORIGIN, SLOT0_ORIGIN);
    return ACK_PARAM_ERR;
  }

  uint32_t ints = save_and_disable_interrupts();

  flash_range_program(addr, packet->data.flash_write.data, MAX_FLASH_DATA);
  
  restore_interrupts(ints);

  *out_len = 0;

  return ACK_OK;
}

uint8_t
get_running_slot_handle(packet_t *in_packet, packet_t *out_packet, uint16_t *out_len)
{
  if ( in_packet->header.length != 0 ) {
    return ACK_LEN_ERR;
  }

  read_running_slot_resp_t *resp = (read_running_slot_resp_t *)out_packet->data.buf;

  static_assert(CUR_SLOT_ORIGIN == SLOT0_ORIGIN ||
                CUR_SLOT_ORIGIN == SLOT1_ORIGIN ||
                CUR_SLOT_ORIGIN == SLOT2_ORIGIN);

  resp->slot_id = (CUR_SLOT_ORIGIN - SLOT0_ORIGIN)/(512*1024);
  *out_len = 1;

  return ACK_OK;
}

uint8_t
set_active_slot_handle(packet_t *in_packet, packet_t *out_packet, uint16_t *out_len)
{
  if ( in_packet->header.length != sizeof(set_active_slot_t) ) {
    return ACK_LEN_ERR;
  }

  set_active_slot_t *req = (set_active_slot_t *)in_packet->data.buf;

  if (set_running_slot(req->slot_id)) {
    return ACK_PARAM_ERR;
  }

  dbg_printf("Header: %02X\n"
             "Msg Id: %04X\n"
             "Length: %04X\n"
             "Slot_Id:%02X\n",
             in_packet->header.cmd_ack,
             in_packet->header.msg_id,
             in_packet->header.length,
             req->slot_id);

  *out_len=0;

  return ACK_OK;
}

uint8_t
reset_handle(packet_t *in_packet, packet_t *out_packet, uint16_t *out_len)
{
  if ( in_packet->header.length != 0 ) {
    return ACK_LEN_ERR;
  }

  reset_pico();

  return ACK_OK;
}
