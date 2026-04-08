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

void epd_init(void);
void epd_reset(void);
void epd_wait_until_idle(void);
void epd_send_command(uint8_t command);
void epd_send_data(const uint8_t *data, int size);
void epd_display_image(const uint8_t *image);

#endif
