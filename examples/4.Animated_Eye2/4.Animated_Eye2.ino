#include "Arduino.h"
#include "lvgl.h"
#include "TFT_eSPI.h"

TFT_eSPI tft = TFT_eSPI(160, 160);
static const uint16_t screenWidth = 160;
static const uint16_t screenHeight = 160;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenHeight * screenWidth / 15];

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
  tft.pushColors(&color_p->full, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1), true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
  printf("LVGL_disp_flush\n");
}
LV_IMG_DECLARE(AA);
// LV_IMG_DECLARE(FJ1);
static lv_obj_t *logo_img = NULL;
void setup() {
  // put your setup code here, to run once:
  lv_init();
  tft.begin();
// tft.invertDisplay(1);
  lv_disp_draw_buf_init(&draw_buf, buf ,NULL, screenHeight * screenWidth / 15);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = 160;
  disp_drv.ver_res = 160;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;  
  lv_disp_drv_register(&disp_drv);

  logo_img = lv_gif_create(lv_scr_act());
  
  lv_obj_align(logo_img, LV_ALIGN_LEFT_MID, 0, 0);
  lv_gif_set_src(logo_img, &AA);
}

void loop() {
  // put your main code here, to run repeatedly:
  lv_timer_handler();
  delay(5);
}
