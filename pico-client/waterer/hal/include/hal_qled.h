#pragma once
#include <stdint.h>

int hal_qled_init(void);
void hal_qled_set_string(const char *text, uint8_t column);
void hal_qled_draw(void);
void hal_qled_disable(void);

void display_driver_init(void);
void display_driver_disable(void);
void display_driver_render(uint8_t *buf, uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end);