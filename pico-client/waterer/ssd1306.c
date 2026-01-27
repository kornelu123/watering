#include <ctype.h>
#include <stdio.h>

#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "font.h"

#define SSD1306_I2C_CLK             400

#define SSD1306_HEIGHT              32
#define SSD1306_WIDTH               128

#define SET_DISPLAY     0xAE
#define SET_CONTRAST    0x81

#define SSD1306_ADDR    0x3C

#define SSD_SDA_PIN     4
#define SSD_SCL_PIN     5

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

#define SSD1306_PAGE_HEIGHT         _u(8)
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)

#define I2C_INST                    i2c0

char
text_buf[SSD1306_HEIGHT/8][SSD1306_WIDTH] = {
    "          ",
    "          ",
    "          ",
    "          ",
};

struct render_area {
  uint8_t start_col;
  uint8_t end_col;
  uint8_t start_page;
  uint8_t end_page;

  int buflen;
};

struct render_area frame_area = {
  .start_col=0,
  .end_col=SSD1306_WIDTH - 1,
  .start_page=0,
  .end_page=SSD1306_NUM_PAGES - 1,
  .buflen=(((SSD1306_WIDTH)*(SSD1306_NUM_PAGES))),
};

uint8_t buf[((SSD1306_WIDTH + 1)*(SSD1306_NUM_PAGES + 1))];

bool SSD1306_send_cmd(uint8_t cmd) {
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

void
SSD1306_send_cmd_list(uint8_t *buf, int num)
{
  for (int i=0;i<num;i++) {
    SSD1306_send_cmd(buf[i]);
  }
}

bool
SSD1306_send_buf(uint8_t buf[], int buflen)
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

void SSD1306_init() {
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
    SSD1306_HEIGHT - 1,             // Display height - 1
    SSD1306_SET_COM_OUT_DIR | 0x08, // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
    SSD1306_SET_DISP_OFFSET,        // set display offset
    0x00,                           // no offset
    SSD1306_SET_COM_PIN_CFG,        // set COM (common) pins hardware configuration. Board specific magic number.
                                    // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
    0x02,
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
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

  SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void SSD1306_scroll(bool on) {
  // configure horizontal scrolling
  uint8_t cmds[] = {
    SSD1306_SET_HORIZ_SCROLL | 0x00,
    0x00, // dummy byte
    0x00, // start page 0
    0x00, // time interval
    0x03, // end page 3 SSD1306_NUM_PAGES ??
    0x00, // dummy byte
    0xFF, // dummy byte
    SSD1306_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
  };

  SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void render(uint8_t *buf, struct render_area *area) {
  // update a portion of the display with a render area
  uint8_t cmds[] = {
    SSD1306_SET_COL_ADDR,
    area->start_col,
    area->end_col,
    SSD1306_SET_PAGE_ADDR,
    area->start_page,
    area->end_page
  };

  SSD1306_send_cmd_list(cmds, count_of(cmds));
  SSD1306_send_buf(buf, area->buflen);
}

static void set_pixel(uint8_t *buf, int x,int y, bool on) {
  assert(x >= 0 && x < SSD1306_WIDTH && y >=0 && y < SSD1306_HEIGHT);

  // The calculation to determine the correct bit to set depends on which address
  // mode we are in. This code assumes horizontal

  // The video ram on the SSD1306 is split up in to 8 rows, one bit per pixel.
  // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x=0, y=0->
  // byte 1 is x = 1, y=0->7 etc

  // This code could be optimised, but is like this for clarity. The compiler
  // should do a half decent job optimising it anyway.

  const int BytesPerRow = SSD1306_WIDTH ; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

  int byte_idx = (y / 8) * BytesPerRow + x;
  uint8_t byte = buf[byte_idx];

  if (on)
    byte |=  1 << (y % 8);
  else
    byte &= ~(1 << (y % 8));

  buf[byte_idx] = byte;
}

// Basic Bresenhams.
  static void
draw_line(uint8_t *buf, int x0, int y0, int x1, int y1, bool on) 
{
  int dx =  abs(x1-x0);
  int sx = x0<x1 ? 1 : -1;
  int dy = -abs(y1-y0);
  int sy = y0<y1 ? 1 : -1;
  int err = dx+dy;
  int e2;

  while (true) {
    set_pixel(buf, x0, y0, on);
    if (x0 == x1 && y0 == y1)
      break;
    e2 = 2*err;

    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

  static inline int
get_font_index(uint8_t ch)
{
  if (ch >= 'A' && ch <='Z') {
    return  ch - 'A' + 1;
  }
  else if (ch >= '0' && ch <='9') {
    return  ch - '0' + 27;
  }
  else return  0; // Not got that char so space.
}

  void
write_char(uint8_t *buf, int16_t x, int16_t y, uint8_t ch)
{
  if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
    return;

  // For the moment, only write on Y row boundaries (every 8 vertical pixels)
  y = y/8;

  ch = toupper(ch);
  int idx = get_font_index(ch);
  int fb_idx = y * 128 + x;

  for (int i=0;i<8;i++) {
    buf[fb_idx++] = font[idx * 8 + i];
  }
}

  void
write_string(uint8_t *buf, int16_t x, int16_t y, char *str)
{
  // Cull out any string off the screen
  if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
    return;

  while (*str) {
    write_char(buf, x, y, *str++);
    x+=8;
  }
}

int
init_qled(void)
{
  i2c_init(I2C_INST, SSD1306_I2C_CLK * 1000);
  gpio_set_function(SSD_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SSD_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(SSD_SDA_PIN);
  gpio_pull_up(SSD_SCL_PIN);

  SSD1306_init();

  render(buf, &frame_area);

  // intro sequence: flash the screen 3 times
  for (int i = 0; i < 3; i++) {
    SSD1306_send_cmd(SSD1306_SET_ALL_ON);    // Set all pixels on
    sleep_ms(100);
    SSD1306_send_cmd(SSD1306_SET_ENTIRE_ON); // go back to following RAM for pixel state
    sleep_ms(100);
  }

  int y = 0;
  for (uint32_t i = 0 ;i < 8 || y < SSD1306_HEIGHT; i++) {
      write_string(buf, 0, y, text_buf[i]);
      y+=8;
  }

  render(buf, &frame_area);
}

void
set_string(const char *text, const uint8_t column)
{
  assert(column <= 3);

  memcpy(&(text_buf[column]), text, 16);
}

void
draw(void)
{
  for (uint32_t i = 0 ;i < 4; i++) {
      write_string(buf, 5, i*8, text_buf[i]);
  }

  render(buf, &frame_area);
}
