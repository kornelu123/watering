#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "ssd1680.h"
#include "text_renderer.h"

#define MAX_PUMPS 8

#define WAIT_TIME 10000

extern volatile bool new_data; 
extern uint16_t current_moisture[MAX_PUMPS];
extern bool is_charging;

int disp_init(void) {
  epd_init();

  clear_buffer();
  draw_string(10, 10, "Waiting for data");
  draw_logo();
  epd_display_image(framebuf);

  return 1000;
}

int disp_task(void)
{
    if (!new_data) {
        return WAIT_TIME;
    }

    new_data = false;
    
    clear_buffer();
    char buf[32];
    snprintf(buf, sizeof(buf), "Charger: %s", is_charging ? "ON" : "OFF");
    draw_string(10, 25, buf);

    int y_start = 50;
    int y_step = 15;

    for (int i = 0; i < MAX_PUMPS; i++) {
        snprintf(buf, sizeof(buf), "CH%d:%4d", i, current_moisture[i]);
        
        // Left column (5px) for CH0-CH3, right column (65px) for CH4-CH7
        int x_pos = (i < 4) ? 5 : 65; 
        int y_pos = y_start + ((i % 4) * y_step);
        
        draw_string(x_pos, y_pos, buf);
    }

    draw_logo();
    epd_display_image(framebuf);
    return WAIT_TIME;
}