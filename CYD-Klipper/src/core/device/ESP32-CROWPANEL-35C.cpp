// Adapted from https://github.com/OzInFl/Elecrow-3.5-RGB-TFT-SQUARELINE-EXAMPLE

#ifdef CYD_SCREEN_DRIVER_ESP32_CROWPANEL_35C
#include "../screen_driver.h"
#include "lvgl.h"
#include "../../conf/global_config.h"
#include <LovyanGFX.hpp>
#include <Arduino.h>
#include <Wire.h>

#ifdef CYD_SCREEN_VERTICAL
#error "Vertical screen not supported with the ESP32_CROWPANEL_28R driver"
#endif

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[CYD_SCREEN_HEIGHT_PX * CYD_SCREEN_WIDTH_PX / 10];

#define BUZZER_PIN 20
#define LCD_BL 46
#define SDA_FT6236 38
#define SCL_FT6236 39
#define I2C_TOUCH_ADDR 0x38

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_Parallel16 _bus_instance;
    lgfx::Touch_FT5x06 _touch_instance;

public:
    LGFX()
    {
        auto bus_cfg = _bus_instance.config();
        bus_cfg.port = 0;
        bus_cfg.freq_write = 80000000;
        bus_cfg.pin_wr = 18;
        bus_cfg.pin_rd = 48;
        bus_cfg.pin_rs = 45;
        bus_cfg.pin_d0 = 47;
        bus_cfg.pin_d1 = 21;
        bus_cfg.pin_d2 = 14;
        bus_cfg.pin_d3 = 13;
        bus_cfg.pin_d4 = 12;
        bus_cfg.pin_d5 = 11;
        bus_cfg.pin_d6 = 10;
        bus_cfg.pin_d7 = 9;
        bus_cfg.pin_d8 = 3;
        bus_cfg.pin_d9 = 8;
        bus_cfg.pin_d10 = 16;
        bus_cfg.pin_d11 = 15;
        bus_cfg.pin_d12 = 7;
        bus_cfg.pin_d13 = 6;
        bus_cfg.pin_d14 = 5;
        bus_cfg.pin_d15 = 4;
        _bus_instance.config(bus_cfg);
        _panel_instance.setBus(&_bus_instance);

        auto panel_cfg = _panel_instance.config();
        panel_cfg.pin_cs = -1;
        panel_cfg.pin_rst = -1;
        panel_cfg.pin_busy = -1;
        panel_cfg.memory_width = 320;
        panel_cfg.memory_height = 480;
        panel_cfg.panel_width = 320;
        panel_cfg.panel_height = 480;
        panel_cfg.offset_x = 0;
        panel_cfg.offset_y = 0;
        panel_cfg.offset_rotation = 0;
        panel_cfg.dummy_read_pixel = 8;
        panel_cfg.dummy_read_bits = 1;
        panel_cfg.readable = true;
        panel_cfg.invert = global_config.printer_config[global_config.printer_index].invert_colors ? true : false;
        panel_cfg.rgb_order = false;
        panel_cfg.dlen_16bit = true;
        panel_cfg.bus_shared = true;

        _panel_instance.config(panel_cfg);

        auto touch_cfg = _touch_instance.config();
        touch_cfg.x_min = 0;
        touch_cfg.x_max = 319;
        touch_cfg.y_min = 0;
        touch_cfg.y_max = 479;
        touch_cfg.pin_int = -1;
        touch_cfg.bus_shared = false;
        touch_cfg.offset_rotation = 0;

        touch_cfg.i2c_port = 1;
        touch_cfg.i2c_addr = 0x38;
        touch_cfg.pin_sda = 38;
        touch_cfg.pin_scl = 39;
        touch_cfg.freq = 400000;

        _touch_instance.config(touch_cfg);
        _panel_instance.setTouch(&_touch_instance);

        setPanel(&_panel_instance);
    }
};

LGFX tft;

void screen_setBrightness(unsigned char brightness)
{
    // TODO
}

void set_invert_display()
{
    // TODO
}

void screen_lv_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void screen_lv_touchRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;

    bool touched = tft.getTouch(&touchX, &touchY);

    if (touchX > CYD_SCREEN_WIDTH_PX || touchY > CYD_SCREEN_HEIGHT_PX)
    {
        LOG_LN("Y or y outside of expected parameters..");
    }
    else
    {
        data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

void screen_setup()
{
    pinMode(BUZZER_PIN, OUTPUT);
    ledcSetup(4, 5000, 8);
    ledcAttachPin(BUZZER_PIN, 4);

    tft.begin();
    tft.setRotation(global_config.rotate_screen ? 3 : 1);

    delay(500);

    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);

/*
    ledcSetup(0, 5000, 12);
    ledcAttachPin(LCD_BL, 0);
*/

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, CYD_SCREEN_WIDTH_PX * CYD_SCREEN_HEIGHT_PX / 10);

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

#endif