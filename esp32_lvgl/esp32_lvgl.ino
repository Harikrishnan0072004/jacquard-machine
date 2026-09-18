/*
 * Jacquard UI on ESP32 DevKit (classic ESP32-WROOM-32)
 * ILI9488 3.5" SPI display (landscape 480x320) + XPT2046 touch + LVGL v9
 *
 * WIRING (display and touch share one SPI bus)
 *   ESP32      Display (ILI9488)     Touch (XPT2046)
 *   3V3   ->   VCC, LED/BL
 *   GND   ->   GND
 *   GPIO18 ->  SCK                   T_CLK
 *   GPIO23 ->  SDI (MOSI)            T_DIN
 *   GPIO19 ->  (leave SDO/MISO of display UNCONNECTED!)   T_DO
 *   GPIO15  ->  CS
 *   GPIO2  ->  DC / RS
 *   GPIO12 ->  RST
 *   GPIO21 ->                        T_CS
 *              T_IRQ: not needed (we detect touch by pressure)
 *
 *   Why leave the display's SDO unconnected? Many ILI9488 boards never release
 *   that line, so the touch chip could not answer on MISO.
 *
 * SKETCH FOLDER must contain: this .ino, ui.h and all the UI .c files.
 * Do NOT copy the simulator's main.c, mouse_cursor_icon.c, CMakeLists.txt, lv_conf.h.
 */

#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>
#include "ui.h"

/* ================= Pins ================= */
#define PIN_SCK        18
#define PIN_MISO       19
#define PIN_MOSI       23
#define PIN_LCD_CS      15
#define PIN_LCD_DC      2
#define PIN_LCD_RST    12
#define PIN_TOUCH_CS   21

/* ================= Display ================= */
#define DISP_HOR_RES   480
#define DISP_VER_RES   320
#define LCD_MADCTL     0x28          /* landscape + BGR. Upside down? try 0xE8 */
#define LCD_SPI_HZ     20000000      /* lower to 10000000 if you see noise    */

/* ================= Touch ================= */
#define USE_TOUCH          1 //1
#define TOUCH_DEBUG        1         /* prints raw values: use them to calibrate */
#define TOUCH_SPI_HZ       2000000   /* XPT2046 is slow: 2 MHz max */
#define TOUCH_Z_THRESHOLD  400       /* bigger = needs a firmer press */

/* Calibration: touch the 4 corners, read the Serial Monitor, update these */
#define TOUCH_RAW_X_MIN    300
#define TOUCH_RAW_X_MAX    3800
#define TOUCH_RAW_Y_MIN    300
#define TOUCH_RAW_Y_MAX    3800
#define TOUCH_SWAP_XY      1         /* landscape usually needs swap */
#define TOUCH_INVERT_X     1 //0 
#define TOUCH_INVERT_Y    1 // 0

static const SPISettings lcdSPI(LCD_SPI_HZ, MSBFIRST, SPI_MODE0);
static const SPISettings touchSPI(TOUCH_SPI_HZ, MSBFIRST, SPI_MODE0);

/* =====================================================================
 *  ILI9488 low level
 *  Rule: these helpers assume an SPI transaction is ALREADY open.
 *  Never call SPI.beginTransaction() twice without endTransaction():
 *  on ESP32 it takes a lock and a nested call would hang forever.
 * ===================================================================== */
static inline void lcd_begin() { SPI.beginTransaction(lcdSPI); }
static inline void lcd_end()   { SPI.endTransaction(); }

static void lcd_cmd(uint8_t cmd)
{
  digitalWrite(PIN_LCD_DC, LOW);
  digitalWrite(PIN_LCD_CS, LOW);
  SPI.write(cmd);
  digitalWrite(PIN_LCD_CS, HIGH);
}

static void lcd_data(const uint8_t *data, size_t len)
{
  digitalWrite(PIN_LCD_DC, HIGH);
  digitalWrite(PIN_LCD_CS, LOW);
  SPI.writeBytes(data, len);
  digitalWrite(PIN_LCD_CS, HIGH);
}

static void lcd_data_byte(uint8_t b) { lcd_data(&b, 1); }

static void ili9488_init()
{
  digitalWrite(PIN_LCD_RST, HIGH); delay(10);
  digitalWrite(PIN_LCD_RST, LOW);  delay(20);
  digitalWrite(PIN_LCD_RST, HIGH); delay(120);

  lcd_begin();

  lcd_cmd(0xE0);
  const uint8_t g1[] = {0x00,0x03,0x09,0x08,0x16,0x0A,0x3F,0x78,0x4C,0x09,0x0A,0x08,0x16,0x1A,0x0F};
  lcd_data(g1, sizeof(g1));

  lcd_cmd(0xE1);
  const uint8_t g2[] = {0x00,0x16,0x19,0x03,0x0F,0x05,0x32,0x45,0x46,0x04,0x0E,0x0D,0x35,0x37,0x0F};
  lcd_data(g2, sizeof(g2));

  lcd_cmd(0xC0);
  const uint8_t p1[] = {0x17, 0x15};
  lcd_data(p1, sizeof(p1));

  lcd_cmd(0xC1); lcd_data_byte(0x41);

  lcd_cmd(0xC5);
  const uint8_t vcom[] = {0x00, 0x12, 0x80};
  lcd_data(vcom, sizeof(vcom));

  lcd_cmd(0x36); lcd_data_byte(LCD_MADCTL);   /* orientation */
  lcd_cmd(0x3A); lcd_data_byte(0x55);  //66        /* 18-bit RGB666 (SPI) */
  lcd_cmd(0xB0); lcd_data_byte(0x00);
  lcd_cmd(0xB1); lcd_data_byte(0xA0);
  lcd_cmd(0xB4); lcd_data_byte(0x02);

  lcd_cmd(0xB6);
  const uint8_t dfc[] = {0x02, 0x02};
  lcd_data(dfc, sizeof(dfc));

  lcd_cmd(0xE9); lcd_data_byte(0x00);

  lcd_cmd(0xF7);
  const uint8_t adj[] = {0xA9, 0x51, 0x2C, 0x82};
  lcd_data(adj, sizeof(adj));

  lcd_cmd(0x11);
  lcd_end();
  delay(120);

  lcd_begin();
  lcd_cmd(0x29);
  lcd_end();
  delay(25);
}

static void ili9488_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
  lcd_cmd(0x2A);
  uint8_t ca[] = {(uint8_t)(x1 >> 8), (uint8_t)x1, (uint8_t)(x2 >> 8), (uint8_t)x2};
  lcd_data(ca, 4);

  lcd_cmd(0x2B);
  uint8_t ra[] = {(uint8_t)(y1 >> 8), (uint8_t)y1, (uint8_t)(y2 >> 8), (uint8_t)y2};
  lcd_data(ra, 4);

  lcd_cmd(0x2C);
}

static uint8_t line_buf[DISP_HOR_RES * 3];   /* one line in RGB666 */

static void fill_color(uint8_t r, uint8_t g, uint8_t b)
{
  for (int x = 0; x < DISP_HOR_RES; x++) {
    line_buf[x * 3] = r; line_buf[x * 3 + 1] = g; line_buf[x * 3 + 2] = b;
  }
  lcd_begin();
  ili9488_set_window(0, 0, DISP_HOR_RES - 1, DISP_VER_RES - 1);
  digitalWrite(PIN_LCD_DC, HIGH);
  digitalWrite(PIN_LCD_CS, LOW);
  for (int y = 0; y < DISP_VER_RES; y++) SPI.writeBytes(line_buf, sizeof(line_buf));
  digitalWrite(PIN_LCD_CS, HIGH);
  lcd_end();
}

/* =====================================================================
 *  LVGL display glue
 * ===================================================================== */
static uint32_t my_tick() { return (uint32_t)millis(); }

static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;

  lcd_begin();
  ili9488_set_window(area->x1, area->y1, area->x2, area->y2);
  digitalWrite(PIN_LCD_DC, HIGH);
  digitalWrite(PIN_LCD_CS, LOW);
  for (int32_t y = 0; y < h; y++) {
    for (int32_t x = 0; x < w; x++) {
      uint16_t c = px_map[0] | (px_map[1] << 8);   /* RGB565, little-endian */
      px_map += 2;
      line_buf[x * 3]     = (c >> 8) & 0xF8;
      line_buf[x * 3 + 1] = (c >> 3) & 0xFC;
      line_buf[x * 3 + 2] = (c << 3) & 0xF8;
    }
    SPI.writeBytes(line_buf, w * 3);
  }
  digitalWrite(PIN_LCD_CS, HIGH);
  lcd_end();

  lv_display_flush_ready(disp);
}

/* 40 lines x 480 px x 2 bytes = 38,400 bytes */
//static uint8_t draw_buf[DISP_HOR_RES * 40 * 2];
// #define DRAW_BUF_SIZE (DISP_HOR_RES * 40 * 2)
// static uint8_t *draw_buf = NULL;
#define DRAW_BUF_LINES  32
#define DRAW_BUF_SIZE   (DISP_HOR_RES * DRAW_BUF_LINES * 2)
static uint8_t draw_buf[DRAW_BUF_SIZE];
/* =====================================================================
 *  XPT2046 touch
 * ===================================================================== */
#if USE_TOUCH
static uint16_t xpt_read12(uint8_t cmd)
{
  SPI.transfer(cmd);
  return SPI.transfer16(0) >> 3;      /* 12-bit result */
}

/* Returns true while pressed; raw values in rx, ry */
/* Returns true while pressed; raw values in rx, ry */
static bool touch_get_raw(uint16_t *rx, uint16_t *ry)
{
  SPI.beginTransaction(touchSPI);
  digitalWrite(PIN_TOUCH_CS, LOW);          // touch chip: listen to me

  int32_t z1 = xpt_read12(0xB1);
  int32_t z2 = xpt_read12(0xC1);
  int32_t z  = z1 + 4095 - z2;
  bool pressed = (z1 > 100) && (z > TOUCH_Z_THRESHOLD);

  /* ---------- NEW DEBUG BLOCK (replaces the old one) ---------- */
#if TOUCH_DEBUG
  static uint32_t zdbg = 0;
  if (millis() - zdbg > 300) {
    zdbg = millis();
    uint16_t dx = xpt_read12(0xD1);
    uint16_t dy = xpt_read12(0x91);
    Serial.printf("z1=%4ld z2=%4ld z=%5ld x=%4u y=%4u pressed=%d\n",
                  z1, z2, z, dx, dy, pressed);
  }
#endif
  /* ------------------------------------------------------------ */

  uint32_t sx = 0, sy = 0;
  if (pressed) {
    xpt_read12(0x91);                       // first sample is noisy, discard
    for (int i = 0; i < 4; i++) {
      sx += xpt_read12(0xD1);               // X
      sy += xpt_read12(0x91);               // Y
    }
  }
  xpt_read12(0xD0);                         // power down between readings

  digitalWrite(PIN_TOUCH_CS, HIGH);         // touch chip: done
  SPI.endTransaction();

  *rx = sx / 4;
  *ry = sy / 4;
  return pressed;
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
  (void)indev;
  uint16_t rx, ry;

  if (!touch_get_raw(&rx, &ry)) {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

#if TOUCH_DEBUG
  static uint32_t last_print = 0;
  if (millis() - last_print > 200) {
    last_print = millis();
    Serial.printf("touch raw x=%u y=%u\n", rx, ry);
  }
#endif

#if TOUCH_SWAP_XY
  uint16_t t = rx; rx = ry; ry = t;
#endif

  int32_t x = map(rx, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, DISP_HOR_RES - 1);
  int32_t y = map(ry, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, DISP_VER_RES - 1);
#if TOUCH_INVERT_X
  x = (DISP_HOR_RES - 1) - x;
#endif
#if TOUCH_INVERT_Y
  y = (DISP_VER_RES - 1) - y;
#endif

  data->point.x = constrain(x, 0, DISP_HOR_RES - 1);
  data->point.y = constrain(y, 0, DISP_VER_RES - 1);
  data->state   = LV_INDEV_STATE_PRESSED;
}
#endif /* USE_TOUCH */

/* =====================================================================
 *  LVGL log -> Serial
 * ===================================================================== */
#if LV_USE_LOG
static void my_log_cb(lv_log_level_t level, const char *buf)
{
  (void)level;
  Serial.print(buf);
}
#endif

/* ===================================================================== */
void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("\nJacquard UI - ESP32 + ILI9488 + LVGL");

  pinMode(PIN_LCD_CS, OUTPUT);   digitalWrite(PIN_LCD_CS, HIGH);
  pinMode(PIN_TOUCH_CS, OUTPUT); digitalWrite(PIN_TOUCH_CS, HIGH);
  pinMode(PIN_LCD_DC, OUTPUT);
  pinMode(PIN_LCD_RST, OUTPUT);

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);   /* CS pins handled manually */

  ili9488_init();
  fill_color(0xFC, 0x00, 0x00); delay(300);  /* quick wiring check */
  fill_color(0x00, 0xFC, 0x00); delay(300);
  fill_color(0x00, 0x00, 0xFC); delay(300);

  lv_init();
  lv_tick_set_cb(my_tick);
#if LV_USE_LOG
  lv_log_register_print_cb(my_log_cb);
#endif

  lv_display_t *disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(disp, draw_buf, NULL,    DRAW_BUF_SIZE,  LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);

#if USE_TOUCH
  lv_indev_t *touch = lv_indev_create();
  lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(touch, touch_read_cb);
#endif

  my_app_init();   /* your UI: boot screen -> main menu */

  Serial.printf("Setup done. Free heap: %u bytes\n", ESP.getFreeHeap());
Serial.printf("Draw buffer: %u bytes (%d lines of %d px)\n",
              (unsigned)DRAW_BUF_SIZE, DRAW_BUF_LINES, DISP_HOR_RES);
Serial.printf("Largest free block: %u\n",
              heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
Serial.printf("bytes per pixel = %d\n",
  (int)lv_color_format_get_size(lv_display_get_color_format(disp)));
}

void loop()
{
  lv_timer_handler();

  /* Leak detector: this number must stay stable while you navigate */
  static uint32_t last = 0;
  if (millis() - last > 5000) {
    last = millis();
    Serial.printf("Free heap: %u  (min ever: %u)\n", ESP.getFreeHeap(), ESP.getMinFreeHeap());
  }

  delay(5);
}
