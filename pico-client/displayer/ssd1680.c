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

    spi_ink_write_data(CMD_SW_RESET, NULL, 0);
    epd_wait_until_idle();
    sleep_ms(100);

    buf[0] = 0x41; // VSH1 15V
    buf[1] = 0x00; // VSH2 ignored
    buf[2] = 0x32; // VSL -15V
    spi_ink_write_data(CMD_SOURCE_DRIVING_VOLTAGE_CONTROL, buf, 3);

    // Set gate driver output

    // MUX ratio
    buf[0] = HEIGHT - 1;
    buf[1] = (HEIGHT + 1) >> 8;
    
    buf[2] = 0x00; // Default scanning direction
    spi_ink_write_data(CMD_DRIVER_OUTPUT_CONTROL, buf, 3);

    buf[0] = 0xF7; // Full refresh
    spi_ink_write_data(CMD_DISPLAY_UPDATE_CONTROL, buf, 1);
    spi_ink_write_data(CMD_MASTER_ACTIVATION, NULL, 0);

    //Set panel border
    buf[0] = 0x05; // Follow LUT1 signal
    spi_ink_write_data(CMD_BORDER_WAVEFORM_CONTROL, buf, 1);

    buf[0] = 0x78; // -3.0V
    spi_ink_write_data(CMD_WRITE_VCOM_REGISTER, buf, 1);
}


void epd_display_image(const uint8_t *framebuf){

    // Set RAM pointers to 0
    buf[0] = 0x00;
    buf[1] = 0x00;
    spi_ink_write_data(CMD_SET_RAM_X_ADDRESS_COUNTER, buf, 1);
    

    spi_ink_write_data(CMD_SET_RAM_Y_ADDRESS_COUNTER, buf, 2);

    memcpy(buf, framebuf, 4000);
    uint16_t size = (WIDTH*HEIGHT)/8;
    spi_ink_write_data(CMD_WRITE_RAM, buf, size);
    sleep_ms(10);


    //Update the display
    buf[0] = 0xF7; // Full refresh
    spi_ink_write_data(CMD_DISPLAY_UPDATE_CONTROL, buf, 1);
    spi_ink_write_data(CMD_MASTER_ACTIVATION, NULL, 0);

    epd_wait_until_idle();
}

int epd_is_busy(void) {
    return gpio_get(BUSY_PIN) == 1;
}

void epd_write_framebuffer(const uint8_t *framebuf) {
    buf[0] = 0x00;
    buf[1] = 0x00;
    spi_ink_write_data(CMD_SET_RAM_X_ADDRESS_COUNTER, buf, 1);     
    
    buf[0] = 0x00;
    buf[1] = 0x00;
    spi_ink_write_data(CMD_SET_RAM_Y_ADDRESS_COUNTER, buf, 2);     

    memcpy(buf, framebuf, 4000);
    uint16_t size = (WIDTH*HEIGHT)/8;
    spi_ink_write_data(CMD_WRITE_RAM, buf, size);    

}

void epd_trigger_refresh(void){
    buf[0] = 0xF7; // Full refresh
    spi_ink_write_data(CMD_DISPLAY_UPDATE_CONTROL, buf, 1);
    spi_ink_write_data(CMD_MASTER_ACTIVATION, NULL, 0);
}