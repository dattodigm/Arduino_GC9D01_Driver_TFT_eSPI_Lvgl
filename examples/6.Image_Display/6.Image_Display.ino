#include <lvgl.h>
#include <TFT_eSPI.h>
#include "Arduino.h"
#include "lvgl.h"


/*Change to your screen resolution*/
static const uint16_t screenWidth  = 160;
static const uint16_t screenHeight = 160;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 10 ];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
static void lv_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h);
  lv_disp_flush_ready(disp);
}

void setup()
{
    Serial.begin( 115200 ); /* prepare for possible serial debug */

    lv_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print ); /* register print function for debugging */
#endif

    tft.begin();          /* TFT init */
    tft.setRotation( 0 ); /* Landscape orientation, flipped */

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * screenHeight / 10 );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = lv_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register( &disp_drv );

    LV_IMG_DECLARE(A3);
    lv_obj_t *logo_img = lv_img_create(lv_scr_act());
    lv_img_set_src(logo_img,&A3);
    lv_obj_center(logo_img);
    lv_obj_align(logo_img, LV_ALIGN_LEFT_MID, 0, 0);
    // lv_gif_set_src(logo_img, &logob);
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}

