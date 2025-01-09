#ifdef CYD_SCREEN_DRIVER_ESP32_CROWPANEL_28R
#include "../screen_driver.h"

#ifdef CYD_SCREEN_VERTICAL
    #error "Vertical screen not supported with the ESP32_CROWPANEL_28R driver"
#endif

#include <SPI.h>
#include <TFT_eSPI.h>
#include "../../conf/global_config.h"
#include "lvgl.h"
#include <TFT_eSPI.h>
#include "../lv_setup.h"

#define TOUCH_THRESHOLD 600

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[CYD_SCREEN_HEIGHT_PX * CYD_SCREEN_WIDTH_PX / 10];

TFT_eSPI tft = TFT_eSPI();

uint16_t touchX, touchY;

void screen_setBrightness(byte brightness)
{
    // calculate duty, 4095 from 2 ^ 12 - 1
    uint32_t duty = (4095 / 255) * brightness;

    // write duty to LEDC
    ledcWrite(0, duty);
}

void screen_lv_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void screen_lv_touchRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    if (tft.getTouch( &touchX, &touchY, TOUCH_THRESHOLD))
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void set_invert_display(){
    tft.invertDisplay(global_config.printer_config[global_config.printer_index].invert_colors);
}

void screen_setup()
{
    uint16_t calData[5] = { 189, 3416, 359, 3439, 1};

    lv_init();

    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.invertDisplay(false);
    delay(300);

    tft.setRotation(1);
    tft.setTouch( calData );

    ledcSetup(0, 5000, 12);
    ledcAttachPin(TFT_BL, 0);

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, CYD_SCREEN_HEIGHT_PX * CYD_SCREEN_WIDTH_PX / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = CYD_SCREEN_WIDTH_PX;
    disp_drv.ver_res = CYD_SCREEN_HEIGHT_PX;
    disp_drv.flush_cb = screen_lv_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = screen_lv_touchRead;
    lv_indev_drv_register(&indev_drv);
}

#endif // CYD_SCREEN_DRIVER_ESP32_CROWPANEL_28R