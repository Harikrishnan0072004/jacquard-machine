#include <lvgl.h>
#include <TFT_eSPI.h>
#include "ui.h"

TFT_eSPI tft;

#define BUF_LINES 40

static uint8_t draw_buf[UI_HOR_RES * BUF_LINES * 2] __attribute__((aligned(64)));

static uint32_t my_tick(void) { return (uint32_t)millis(); }

static void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, true);   // use the value that gave you correct colors
  tft.endWrite();

  lv_display_flush_ready(disp);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  tft.init();
  tft.setRotation(1);              // use the value that looked right for you
  tft.fillScreen(TFT_BLACK);

  lv_init();
  lv_tick_set_cb(my_tick);

  lv_display_t *disp = lv_display_create(UI_HOR_RES, UI_VER_RES);
  if (!disp) { Serial.println("display create FAILED"); while (1) delay(1000); }

  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, my_flush_cb);
  lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  my_app_init();                   // boot screen -> main menu
  Serial.println("UI started");
}

void loop() {
  lv_timer_handler();
  delay(5);
}
