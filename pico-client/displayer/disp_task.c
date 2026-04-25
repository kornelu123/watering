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
extern bool pump_active[MAX_PUMPS];
extern uint8_t battery_level;
extern uint8_t water_level;

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
    snprintf(buf, sizeof(buf), "Bat: %d%%  Water: %d%%", battery_level, water_level);
    draw_string(10, 10, buf);

    int y_start = 40;
    int y_step = 20;
    int drawn = 0;

    for (int i = 0; i < MAX_PUMPS; i++) {
        if(!pump_active[i]){
            continue;
        }

        snprintf(buf, sizeof(buf), "CH%d:%4d", i, current_moisture[i]);
        
        int x_pos = (drawn/4) * 60 + 5;
        int y_pos = y_start + ((drawn % 4) * y_step);
        
        draw_string(x_pos, y_pos, buf);
        drawn++;
    }

    if (drawn == 0) {
        draw_string(10, 60, "No active pumps");
    }

    draw_logo();
    epd_display_image(framebuf);
    return WAIT_TIME;
}