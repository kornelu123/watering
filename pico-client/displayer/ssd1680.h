#ifndef SSD1680_H
#define SSD1680_H

#include "pico/stdlib.h"

#define SPI_RX_PIN      16
#define SPI_SCK_PIN     18
#define SPI_TX_PIN      19

#define DC_PIN          20
#define RST_PIN         21
#define BUSY_PIN        22
#define EN_PIN          26
#define SRAM_CS_PIN     27
#define EINK_CS_PIN     17

#define HEIGHT 250
#define WIDTH 128

#define CMD_DRIVER_OUTPUT_CONTROL          0x01
#define CMD_SOURCE_DRIVING_VOLTAGE_CONTROL 0x04
#define CMD_SW_RESET                       0x12
#define CMD_MASTER_ACTIVATION              0x20
#define CMD_DISPLAY_UPDATE_CONTROL         0x22
#define CMD_WRITE_RAM                      0x24
#define CMD_WRITE_VCOM_REGISTER            0x2C
#define CMD_BORDER_WAVEFORM_CONTROL        0x3C
#define CMD_SET_RAM_X_ADDRESS_COUNTER      0x4E
#define CMD_SET_RAM_Y_ADDRESS_COUNTER      0x4F

void epd_init(void);
void epd_reset(void);
void epd_wait_until_idle(void);
int  epd_is_busy(void); 
void epd_write_framebuffer(const uint8_t *framebuf);
void epd_trigger_refresh(void);

#endif
