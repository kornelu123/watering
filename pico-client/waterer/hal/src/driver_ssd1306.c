#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pins.h"

#define DISPLAY_HEIGHT       32
#define DISPLAY_WIDTH        128
#define DISPLAY_PAGE_HEIGHT  8
#define DISPLAY_NUM_PAGES    (DISPLAY_HEIGHT / DISPLAY_PAGE_HEIGHT)
#define DISPLAY_BUF_LEN      (DISPLAY_NUM_PAGES * DISPLAY_WIDTH)

#define SSD1306_I2C_CLK             400

#define SSD1306_ADDR    0x3C

#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)

#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)

#define I2C_INST                    i2c0

static bool
driver_send_cmd(uint8_t cmd)
{
  uint8_t buf[2] = {0x00, cmd};  // Control byte 0x00 for commands
  int result = i2c_write_timeout_us(I2C_INST, SSD1306_ADDR, buf, 2, false, 100000);  // 100ms timeout

  if (result == PICO_ERROR_TIMEOUT) {
    // Reset I2C on timeout
    i2c_hw_t *hw = I2C_INST->hw;
    hw->enable = 0;
    hw->enable = 1;
    return false;
  }
  return result == 2;
}

static void
driver_send_cmd_list(uint8_t *buf, int num)
{
  for (int i=0;i<num;i++) {
    driver_send_cmd(buf[i]);
  }
}

static bool
driver_send_buf(uint8_t *buf, int buflen)
{
  uint8_t *temp_buf = malloc(buflen + 1);
  if (!temp_buf) return false;

  temp_buf[0] = 0x40;  // Control byte 0x40 for data
  memcpy(temp_buf + 1, buf, buflen);

  int result = i2c_write_timeout_us(I2C_INST, SSD1306_ADDR, temp_buf, buflen + 1, false, 500000);  // 500ms timeout
  free(temp_buf);

  if (result == PICO_ERROR_TIMEOUT) {
    // Reset I2C
    i2c_hw_t *hw = I2C_INST->hw;
    hw->enable = 0;
    hw->enable = 1;
    return false;
  }
  return result == (buflen + 1);
}

void
display_driver_init(void)
{
  i2c_init(I2C_INST, SSD1306_I2C_CLK * 1000);
  gpio_set_function(SSD_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SSD_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(SSD_SDA_PIN);
  gpio_pull_up(SSD_SCL_PIN);

  // Some of these commands are not strictly necessary as the reset
  // process defaults to some of these but they are shown here
  // to demonstrate what the initialization sequence looks like
  // Some configuration values are recommended by the board manufacturer

  uint8_t cmds[] = {
    SSD1306_SET_DISP,               // set display off
    /* memory mapping */
    SSD1306_SET_MEM_MODE,           // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
    0x00,                           // horizontal addressing mode
    /* resolution and layout */
    SSD1306_SET_DISP_START_LINE,    // set display start line to 0
    SSD1306_SET_SEG_REMAP | 0x01,   // set segment re-map, column address 127 is mapped to SEG0
    SSD1306_SET_MUX_RATIO,          // set multiplex ratio
    DISPLAY_HEIGHT - 1,             // Display height - 1
    SSD1306_SET_COM_OUT_DIR | 0x08, // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
    SSD1306_SET_DISP_OFFSET,        // set display offset
    0x00,                           // no offset
    SSD1306_SET_COM_PIN_CFG,        // set COM (common) pins hardware configuration. Board specific magic number.
                                    // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
#if ((DISPLAY_WIDTH == 128) && (DISPLAY_HEIGHT == 32))
    0x02,
#elif ((DISPLAY_WIDTH == 128) && (DISPLAY_HEIGHT == 64))
    0x12,
#else
    0x02,
#endif
    /* timing and driving scheme */
    SSD1306_SET_DISP_CLK_DIV,       // set display clock divide ratio
    0x80,                           // div ratio of 1, standard freq
    SSD1306_SET_PRECHARGE,          // set pre-charge period
    0xF1,                           // Vcc internally generated on our board
    SSD1306_SET_VCOM_DESEL,         // set VCOMH deselect level
    0x30,                           // 0.83xVcc
    /* display */
    SSD1306_SET_CONTRAST,           // set contrast control
    0xFF,
    SSD1306_SET_ENTIRE_ON,          // set entire display on to follow RAM content
    SSD1306_SET_NORM_DISP,           // set normal (not inverted) display
    SSD1306_SET_CHARGE_PUMP,        // set charge pump
    0x14,                           // Vcc internally generated on our board
    SSD1306_SET_SCROLL | 0x00,      // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
    SSD1306_SET_DISP | 0x01, // turn display on
  };

  driver_send_cmd_list(cmds, count_of(cmds));
}

void
display_driver_disable(void)
{
  uint8_t cmd = SSD1306_SET_DISP & ~0x01; // turn display off

  driver_send_cmd(cmd);
}

void
display_driver_render(uint8_t *buf, uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end)
{
    uint8_t start_page = y_start / DISPLAY_PAGE_HEIGHT;
    uint8_t end_page   = y_end / DISPLAY_PAGE_HEIGHT;

    int buflen = (x_end - x_start + 1) * (end_page - start_page + 1);

    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        x_start,
        x_end,
        SSD1306_SET_PAGE_ADDR,
        start_page,
        end_page
    };

    driver_send_cmd_list(cmds, count_of(cmds));
    driver_send_buf(buf, buflen);
}

void
display_driver_scroll(bool on)
{
  // configure horizontal scrolling
  uint8_t cmds[] = {
    SSD1306_SET_HORIZ_SCROLL | 0x00,
    0x00, // dummy byte
    0x00, // start page 0
    0x00, // time interval
    0x03, // end page 3 DISPLAY_NUM_PAGES ??
    0x00, // dummy byte
    0xFF, // dummy byte
    SSD1306_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
  };

  driver_send_cmd_list(cmds, count_of(cmds));
}
