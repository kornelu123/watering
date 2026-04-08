#include "ssd1680.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

uint8_t buf[0x1200];

void epd_reset(){
    gpio_put(RST_PIN,0);
    sleep_ms(10);
    gpio_put(RST_PIN,1);
    sleep_ms(10);
}

void epd_wait_until_idle(){
    while (gpio_get(BUSY_PIN)){
        sleep_ms(10);
    }
}
    
void spi_ink_write_data(uint8_t cmd, uint8_t *data, uint16_t d_len) {

  gpio_put(EINK_CS_PIN, 0);
  gpio_put(DC_PIN, 0);

  spi_write_blocking(spi0, &cmd, 1);

  gpio_put(DC_PIN, 1);

  spi_write_blocking(spi0, data, d_len);

  gpio_put(DC_PIN, 0);
  gpio_put(EINK_CS_PIN, 1);
}


void epd_init(){
    spi_init(spi0, 10 * 1000 * 1000);
    
    gpio_set_function(SPI_TX_PIN,  GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_RX_PIN,  GPIO_FUNC_SPI); 
    
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    gpio_init(EINK_CS_PIN);
    gpio_set_dir(EINK_CS_PIN, GPIO_OUT);
    gpio_put(EINK_CS_PIN, 1);
    gpio_init(DC_PIN);
    gpio_set_dir(DC_PIN, GPIO_OUT);
    gpio_put(DC_PIN, 0);
    gpio_init(EN_PIN);
    gpio_set_dir(EN_PIN, GPIO_OUT);
    gpio_put(EN_PIN, 1);
    
    sleep_ms(100);
    
    gpio_init(RST_PIN);
    gpio_set_dir(RST_PIN, GPIO_OUT);
    gpio_put(RST_PIN, 0);
    sleep_ms(20);
    gpio_init(RST_PIN);
    gpio_set_dir(RST_PIN, GPIO_OUT);
    gpio_put(RST_PIN, 1);
    sleep_ms(200);
    
    gpio_init(SRAM_CS_PIN);
    gpio_set_dir(SRAM_CS_PIN, GPIO_OUT);
    gpio_put(SRAM_CS_PIN, 1);
    gpio_init(BUSY_PIN);
    gpio_set_dir(BUSY_PIN, GPIO_IN);

    epd_reset();

    spi_ink_write_data(0x12, NULL, 0);
    epd_wait_until_idle();
    sleep_ms(100);

    buf[0] = 0x41;
    buf[1] = 0x00;
    buf[2] = 0x32;
    spi_ink_write_data(0x04, buf, 3);  // Source voltage

    // Set gate driver output
    buf[0] = HEIGHT - 1;
    buf[1] = (HEIGHT + 1) >> 8;
    buf[2] = 0x00;
    spi_ink_write_data(0x01, buf, 1);

    buf[0] = 0xF7;
    spi_ink_write_data(0x22, buf, 1);
    spi_ink_write_data(0x20, NULL, 0);

    //Set panel border
    buf[0] = 0x05;
    spi_ink_write_data(0x3C, buf, 1);

    buf[0] = 0x78;
    spi_ink_write_data(0x2C, buf, 1);
}


void epd_display_image(const uint8_t *framebuf){

    // Set RAM pointers to 0
    buf[0] = 0x00;
    buf[1] = 0x00;
    spi_ink_write_data(0x4E, buf, 1);     // Set RAM X adress
    

    spi_ink_write_data(0x4F, buf, 2);     // Set RAM Y adress

    memcpy(buf, framebuf, 4000);
    uint16_t size = (WIDTH*HEIGHT)/8;
    spi_ink_write_data(0x24, buf, size);  // Write to RAM (Black & White only)    
    sleep_ms(10);


    //Update the display
    buf[0] = 0xF7; // Full refresh
    spi_ink_write_data(0x22, buf, 1);
    spi_ink_write_data(0x20, NULL, 0);

    epd_wait_until_idle();
}