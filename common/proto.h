#pragma once

#define MAX_FLASH_DATA                  1024
#define ADDR_SIZE                       3

#define MAX_DATA_LEN                    (MAX_FLASH_DATA + ADDR_SIZE)

#define BUF_LEN                         sizeof(packet_t)
#define HEADER_LEN                      sizeof(header_t)

// Packet headers
#define RESET_PICO_CMD                  0xE0

#define READ_RUNNING_SLOT_CMD           0xB0
#define SET_ACTIVE_SLOT_CMD             0xB1

#define FLASH_WRITE_CMD                 0xF0
#define FLASH_ERASE_CMD                 0xF1

#define IS_ERR_ACK(x)                   (x >= ACK_FIRST_ERR)

#define ACK_FIRST_ERR                   0xFB
#define ACK_PARAM_ERR                   0xFB
#define ACK_CMD_ERR                     0xFC
#define ACK_LEN_ERR                     0xFD

#define ACK_OK                          0x00

#include <stdint.h>

#pragma pack(push,1)

typedef struct header {
  uint8_t cmd_ack;
  uint16_t msg_id;
  uint16_t length;
} header_t;

typedef struct write_flash {
  uint8_t addr[ADDR_SIZE];
  uint8_t data[MAX_FLASH_DATA];
} write_flash_data_t;

typedef struct erase_flash {
  uint8_t addr[ADDR_SIZE];
} erase_flash_data_t;

typedef struct read_running_slot  {
  uint8_t slot_id;
} read_running_slot_resp_t;

typedef struct set_active_slot  {
  uint8_t slot_id;
} set_active_slot_t;

typedef union packet_data {
  uint8_t buf[MAX_DATA_LEN];
  write_flash_data_t flash_write;
  erase_flash_data_t flash_erase;
  read_running_slot_resp_t read_running_slot;
  set_active_slot_t set_active_slot;
} packet_data_t;

typedef struct packet {
  header_t header; 
  packet_data_t data;
} packet_t;

#pragma pack(pop)

uint32_t get_be24(uint8_t *addr_data);
void put_be24(uint32_t addr, uint8_t *addr_data);
