#include "waterer.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

#define SLEEP_TIME 1800000

#define SLEEP_TIME 1800000 // 30min

static uint8_t active_pumps_mask = 0xFF;

void w_init(void) {
    stdio_init_all();
    // ADC
    adc_init();
    adc_gpio_init(ADC0); 
    adc_gpio_init(ADC1);


    // SPI
    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS); 
    gpio_set_dir(PIN_CS, GPIO_OUT); 
    gpio_put(PIN_CS, 1);

    // Decoder
    gpio_init(DEC_ADDR_A);
    gpio_set_dir(DEC_ADDR_A, GPIO_OUT);
    gpio_init(DEC_ADDR_B);
    gpio_set_dir(DEC_ADDR_B, GPIO_OUT);
    gpio_init(DEC_ADDR_C);
    gpio_set_dir(DEC_ADDR_C, GPIO_OUT);
    gpio_init(DEC_ENABLE);
    gpio_set_dir(DEC_ENABLE, GPIO_OUT); 
    gpio_put(DEC_ENABLE, 0);

    // Power management
    gpio_init(PIN_12V_ENABLE);
    gpio_set_dir(PIN_12V_ENABLE, GPIO_OUT);
    gpio_put(PIN_12V_ENABLE, 0); // Turned off

    gpio_init(PIN_CHG_STATUS);
    gpio_set_dir(PIN_CHG_STATUS, GPIO_IN);
}

static int _read_adc_raw(uint8_t channel) {
    uint8_t tx[3] = {
        0x01,
        (uint8_t)(0x80 | ((channel & 0x07) << 4)),
        0x00
    };
    uint8_t rx[3] = {0};

    gpio_put(PIN_CS, 0);
    spi_write_read_blocking(SPI_PORT, tx, rx, 3);
    gpio_put(PIN_CS, 1);

    return ((rx[1] & 0x03) << 8) | rx[2];
}

uint16_t w_read_moisture(uint8_t channel_id) {
    long sum = 0;
    const int samples = 10;
    // Return the average of 10 samples
    for(int i=0; i<samples; i++){
        sum += _read_adc_raw(channel_id);
        sleep_ms(2);
    }
    return (uint16_t)(sum / samples);
}

uint16_t w_read_battery(void) {
    adc_select_input(0);
    uint16_t raw_val = adc_read();
    return (uint8_t)((raw_val * 100) / 4095);
}

uint16_t w_read_water_level(void) {
    adc_select_input(1);
    uint16_t raw_val = adc_read();
    return (uint8_t)((raw_val * 100) / 4095);
}

uint8_t w_get_active_pumps_mask(void) {
    return active_pumps_mask;
}

void w_pump_on(uint8_t channel_id) {
    if (channel_id >= MAX_PUMPS) return;

    gpio_put(DEC_ADDR_A, (channel_id & 0x01));
    gpio_put(DEC_ADDR_B, (channel_id & 0x02));
    gpio_put(DEC_ADDR_C, (channel_id & 0x04));

    gpio_put(PIN_12V_ENABLE, 1);
    sleep_ms(20);

    gpio_put(DEC_ENABLE, 1);
}

void w_pumps_off(void) {
    gpio_put(DEC_ENABLE, 0);
    gpio_put(PIN_12V_ENABLE, 0); // For energy efficiency
}