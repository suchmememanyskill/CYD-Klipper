#ifdef CYD_BOARD_JC4827W543C

#include "../screen_driver.h"
#include <Arduino_GFX_Library.h>
#include "lvgl.h"
#include "../lv_setup.h"
#include "../../conf/global_config.h"
#include "ESP32-JC4827W543C_touch.h"
#define CPU_FREQ_HIGH 240
#define CPU_FREQ_LOW 80

Arduino_DataBus *bus = new Arduino_ESP32QSPI(LCD_CS, LCD_CLK, LCD_D0, LCD_D1, LCD_D2, LCD_D3);
Arduino_NV3041A *panel = new Arduino_NV3041A(bus, LCD_RST, 0, true);

static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_color_t disp_draw_buf[LCD_WIDTH * 48];

void screen_setBrightness(uint8_t brightness)
{
    uint32_t duty = 4095 * brightness / 255;
    ledcWrite(0, duty);
}

void screen_lv_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    panel->startWrite();
    panel->setAddrWindow(area->x1, area->y1, w, h);
    panel->writePixels((uint16_t *)&color_p->full, w * h);
    panel->endWrite();
    
    lv_disp_flush_ready(disp);
}

void IRAM_ATTR screen_lv_touchRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    if (touch_has_signal() && touch_touched())
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void set_invert_display()
{
    panel->invertDisplay(global_config.printer_config[global_config.printer_index].invert_colors);
}

void screen_setup()
{
    pinMode(LCD_BL_PIN, OUTPUT);
    ledcSetup(0, 5000, 12);
    ledcAttachPin(LCD_BL_PIN, 0);
    screen_setBrightness(128);

    panel->begin();
    panel->setRotation(global_config.rotate_screen ? 2 : 0);

    touch_init(panel->width(), panel->height(), panel->getRotation());
    lv_init();

    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, LCD_WIDTH * 48);
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = screen_lv_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.hor_res = CYD_SCREEN_WIDTH_PX;
    disp_drv.ver_res = CYD_SCREEN_HEIGHT_PX;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = screen_lv_touchRead;
    lv_indev_drv_register(&indev_drv);
}

#endif // CYD_BOARD_JC4827W543C
