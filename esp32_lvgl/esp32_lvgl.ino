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
 *   GPIO15 ->  CS
 *   GPIO2  ->  DC / RS
 *   GPIO12 ->  RST
 *   GPIO21 ->                        T_CS
 *              T_IRQ: not needed (we detect touch by pressure)
 *
 * DMA VERSION: CS pins are now owned by the ESP-IDF spi_master driver
 * (spi_bus_add_device), not toggled by hand. DC is toggled from a
 * pre_cb, synced to actual hardware transmission, not to when the
 * calling code runs.
 *
 * SKETCH FOLDER must contain: this .ino, ui.h and all the UI .c files.
 */

#include <Arduino.h>
#include <string.h>
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include <lvgl.h>
#include "ui.h"

/* ================= Pins ================= */
#define PIN_SCK        18
#define PIN_MISO       19
#define PIN_MOSI       23
#define PIN_LCD_CS     15
#define PIN_LCD_DC      2
#define PIN_LCD_RST    12
#define PIN_TOUCH_CS   21

/* ================= Display ================= */
#define DISP_HOR_RES   480
#define DISP_VER_RES   320
#define LCD_MADCTL     0x28          /* landscape + BGR. Upside down? try 0xE8 */
#define LCD_SPI_HZ     32000000      /* raise to 40000000 once this works cleanly */

/* ================= Touch ================= */
#define USE_TOUCH          1
#define TOUCH_DEBUG        0
#define TOUCH_SPI_HZ       2000000
#define TOUCH_Z_THRESHOLD  400

#define TOUCH_RAW_X_MIN    300
#define TOUCH_RAW_X_MAX    3800
#define TOUCH_RAW_Y_MIN    300
#define TOUCH_RAW_Y_MAX    3800
#define TOUCH_SWAP_XY      1
#define TOUCH_INVERT_X     1
#define TOUCH_INVERT_Y     1

/* ================= Draw / wire buffers ================= */
#define DRAW_BUF_LINES  32
#define DRAW_BUF_SIZE   (DISP_HOR_RES * DRAW_BUF_LINES * 2)   /* RGB565, LVGL side */
#define WIRE_BUF_BYTES  (DISP_HOR_RES * DRAW_BUF_LINES * 3)   /* RGB666, wire side */

#define LCD_HOST SPI2_HOST   /* "HSPI" peripheral on classic ESP32 */

/* transaction.user bit layout */
#define DC_BIT     0x1   /* DC level to apply just before this transaction sends */
#define NOTIFY_BIT 0x2   /* call lv_display_flush_ready() when this one completes */

static spi_device_handle_t spi_lcd;
static spi_device_handle_t spi_touch;
static uint8_t *wire_buf[2];      /* DMA-capable, ping-pong */
static uint8_t  wire_idx = 0;
static lv_display_t *g_disp;

static uint8_t draw_buf1[DRAW_BUF_SIZE];
static uint8_t draw_buf2[DRAW_BUF_SIZE];

/* timing instrumentation */
static volatile uint32_t flush_cpu_us = 0, flush_count = 0;
static volatile uint32_t dma_us = 0, dma_count = 0;
static volatile uint32_t t_dma_start = 0;

/* =====================================================================
 *  ISR callbacks — keep tiny, IRAM_ATTR keeps them out of flash so
 *  they still run during a flash write / OTA stall.
 * ===================================================================== */
static void IRAM_ATTR lcd_pre_cb(spi_transaction_t *t)
{
  digitalWrite(PIN_LCD_DC, ((intptr_t)t->user) & DC_BIT);
  if (((intptr_t)t->user) & NOTIFY_BIT) t_dma_start = micros();
}

static void IRAM_ATTR lcd_post_cb(spi_transaction_t *t)
{
  if (((intptr_t)t->user) & NOTIFY_BIT) {
    dma_us += micros() - t_dma_start;
    dma_count++;
    lv_display_flush_ready(g_disp);
  }
}

/* =====================================================================
 *  Bus + two devices sharing one bus. The driver owns both CS lines.
 * ===================================================================== */
static void spi_bus_setup()
{
  spi_bus_config_t buscfg = {};
  buscfg.mosi_io_num = PIN_MOSI;
  buscfg.miso_io_num = PIN_MISO;
  buscfg.sclk_io_num = PIN_SCK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = WIRE_BUF_BYTES;
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  spi_device_interface_config_t lcd_cfg = {};
  lcd_cfg.clock_speed_hz = LCD_SPI_HZ;
  lcd_cfg.mode = 0;
  lcd_cfg.spics_io_num = PIN_LCD_CS;
  lcd_cfg.queue_size = 2;
  lcd_cfg.pre_cb  = lcd_pre_cb;
  lcd_cfg.post_cb = lcd_post_cb;
  ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &lcd_cfg, &spi_lcd));

  spi_device_interface_config_t touch_cfg = {};
  touch_cfg.clock_speed_hz = TOUCH_SPI_HZ;
  touch_cfg.mode = 0;
  touch_cfg.spics_io_num = PIN_TOUCH_CS;
  touch_cfg.queue_size = 1;
  ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &touch_cfg, &spi_touch));
}

/* =====================================================================
 *  ILI9488 low level — blocking, used only for init and window
 *  commands (small, infrequent). The big pixel payload is the only
 *  thing that goes through the async path below.
 * ===================================================================== */
static void lcd_cmd(uint8_t cmd)
{
  spi_transaction_t t = {};
  t.length = 8;
  t.tx_buffer = &cmd;
  t.user = (void *)(intptr_t)(0);              /* DC=LOW, no notify */
  spi_device_transmit(spi_lcd, &t);
}

static void lcd_data(const uint8_t *data, size_t len)
{
  spi_transaction_t t = {};
  t.length = len * 8;
  t.tx_buffer = data;
  t.user = (void *)(intptr_t)(DC_BIT);          /* DC=HIGH, no notify */
  spi_device_transmit(spi_lcd, &t);
}

static void lcd_data_byte(uint8_t b) { lcd_data(&b, 1); }

static void ili9488_init()
{
  digitalWrite(PIN_LCD_RST, HIGH); delay(10);
  digitalWrite(PIN_LCD_RST, LOW);  delay(20);
  digitalWrite(PIN_LCD_RST, HIGH); delay(120);

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

  lcd_cmd(0x36); lcd_data_byte(LCD_MADCTL);
  lcd_cmd(0x3A); lcd_data_byte(0x66);           /* 18-bit RGB666 (SPI) */
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
  delay(120);

  lcd_cmd(0x29);
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

static void fill_color(uint8_t r, uint8_t g, uint8_t b)
{
  static uint8_t line_buf[DISP_HOR_RES * 3];
  for (int x = 0; x < DISP_HOR_RES; x++) {
    line_buf[x*3] = r; line_buf[x*3+1] = g; line_buf[x*3+2] = b;
  }
  ili9488_set_window(0, 0, DISP_HOR_RES - 1, DISP_VER_RES - 1);
  for (int y = 0; y < DISP_VER_RES; y++) lcd_data(line_buf, sizeof(line_buf));
}

/* =====================================================================
 *  LVGL display glue
 * ===================================================================== */
static uint32_t my_tick() { return (uint32_t)millis(); }

static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;
  g_disp = disp;

  /* Reap any transaction that already finished (non-blocking, timeout 0).
   * Skip this and the queue below eventually hangs forever — the
   * classic DMA-queue-exhaustion bug. */
  spi_transaction_t *done;
  while (spi_device_get_trans_result(spi_lcd, &done, 0) == ESP_OK) { /* reap only */ }

  uint8_t *buf = wire_buf[wire_idx];
  static spi_transaction_t trans[2];
  spi_transaction_t *t = &trans[wire_idx];
  wire_idx ^= 1;

  uint32_t t0 = micros();
  uint8_t *out = buf;
  for (int32_t i = 0; i < w * h; i++) {
    uint16_t c = px_map[0] | (px_map[1] << 8);
    px_map += 2;
    *out++ = (c >> 8) & 0xF8;
    *out++ = (c >> 3) & 0xFC;
    *out++ = (c << 3) & 0xF8;
  }
  flush_cpu_us += micros() - t0;
  flush_count++;

  ili9488_set_window(area->x1, area->y1, area->x2, area->y2);

  memset(t, 0, sizeof(*t));
  t->length    = (size_t)w * h * 3 * 8;
  t->tx_buffer = buf;
  t->user      = (void *)(intptr_t)(DC_BIT | NOTIFY_BIT);
  spi_device_queue_trans(spi_lcd, t, portMAX_DELAY);   /* only blocks if we're
                                                          outrunning the wire */
  /* lv_display_flush_ready() is called later, by lcd_post_cb(),
   * when the hardware actually finishes — not here. */
}

/* =====================================================================
 *  XPT2046 touch
 * ===================================================================== */
#if USE_TOUCH
static uint16_t xpt_read12(uint8_t cmd)
{
  uint8_t tx[3] = { cmd, 0x00, 0x00 };
  uint8_t rx[3] = { 0 };
  spi_transaction_t t = {};
  t.length = 24; t.rxlength = 24;
  t.tx_buffer = tx; t.rx_buffer = rx;
  spi_device_transmit(spi_touch, &t);
  uint16_t raw16 = ((uint16_t)rx[1] << 8) | rx[2];
  return raw16 >> 3;
}

static bool touch_get_raw(uint16_t *rx, uint16_t *ry)
{
  int32_t z1 = xpt_read12(0xB1);
  int32_t z2 = xpt_read12(0xC1);
  int32_t z  = z1 + 4095 - z2;
  bool pressed = (z1 > 100) && (z > TOUCH_Z_THRESHOLD);

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

  uint32_t sx = 0, sy = 0;
  if (pressed) {
    xpt_read12(0x91);                       /* first sample is noisy, discard */
    for (int i = 0; i < 4; i++) {
      sx += xpt_read12(0xD1);
      sy += xpt_read12(0x91);
    }
  }
  xpt_read12(0xD0);                         /* power down between readings */

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
  Serial.println("\nJacquard UI - ESP32 + ILI9488 + LVGL (DMA)");

  pinMode(PIN_LCD_DC, OUTPUT);
  pinMode(PIN_LCD_RST, OUTPUT);
  /* PIN_LCD_CS / PIN_TOUCH_CS: no pinMode/digitalWrite needed —
   * spi_bus_add_device() configures and owns those pins now. */

  spi_bus_setup();

  wire_buf[0] = (uint8_t *)heap_caps_malloc(WIRE_BUF_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  wire_buf[1] = (uint8_t *)heap_caps_malloc(WIRE_BUF_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!wire_buf[0] || !wire_buf[1]) {
    Serial.println("FATAL: could not allocate DMA wire buffers");
    while (1) delay(1000);
  }

  ili9488_init();
  fill_color(0xFC, 0x00, 0x00); delay(300);
  fill_color(0x00, 0xFC, 0x00); delay(300);
  fill_color(0x00, 0x00, 0xFC); delay(300);

  lv_init();
  lv_tick_set_cb(my_tick);
#if LV_USE_LOG
  lv_log_register_print_cb(my_log_cb);
#endif

  lv_display_t *disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(disp, draw_buf1, draw_buf2, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);

#if USE_TOUCH
  lv_indev_t *touch = lv_indev_create();
  lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(touch, touch_read_cb);
#endif

  my_app_init();   /* your UI: boot screen -> main menu */

  Serial.printf("Setup done. Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Wire buffers: 2 x %u bytes, draw buffers: 2 x %u bytes\n",
                (unsigned)WIRE_BUF_BYTES, (unsigned)DRAW_BUF_SIZE);
  Serial.printf("Largest free block: %u\n",
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}

void loop()
{
  lv_timer_handler();

  static uint32_t last = 0;
  if (millis() - last > 5000) {
    last = millis();
    Serial.printf("Free heap: %u (min ever: %u)\n", ESP.getFreeHeap(), ESP.getMinFreeHeap());
    Serial.printf("flush CPU : %u calls, %u ms total (RGB conversion only)\n",
                  flush_count, flush_cpu_us / 1000);
    Serial.printf("DMA send  : %u calls, %u ms total (real wire time)\n",
                  dma_count, dma_us / 1000);
    flush_count = 0; flush_cpu_us = 0;
    dma_count = 0; dma_us = 0;
  }

  delay(5);
}
