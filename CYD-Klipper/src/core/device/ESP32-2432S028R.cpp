#ifdef CYD_SCREEN_DRIVER_ESP32_2432S028R
#include "../screen_driver.h"

#include <SPI.h>
#include <TFT_eSPI.h>
#include "../../conf/global_config.h"
#include "lvgl.h"
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include "../lv_setup.h"

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define CPU_FREQ_HIGH 240
#define CPU_FREQ_LOW 80

SPIClass touchscreen_spi = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[CYD_SCREEN_HEIGHT_PX * CYD_SCREEN_WIDTH_PX / 10];

TFT_eSPI tft = TFT_eSPI();

TS_Point touchscreen_point()
{
    TS_Point p = touchscreen.getPoint();
    p.x = round((p.x * global_config.screenCalXMult) + global_config.screenCalXOffset);
    p.y = round((p.y * global_config.screenCalYMult) + global_config.screenCalYOffset);
    return p;
}

void touchscreen_calibrate(bool force)
{
    if (global_config.screenCalibrated && !force)
        {
            return;
        }

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 140);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.println("Calibrate Screen");

    TS_Point p;
    int16_t x1, y1, x2, y2;

    while (touchscreen.touched())
        ;
    tft.drawFastHLine(0, 10, 20, ILI9341_WHITE);
    tft.drawFastVLine(10, 0, 20, ILI9341_WHITE);
    while (!touchscreen.touched())
        ;
    delay(50);
    p = touchscreen.getPoint();
    x1 = p.x;
    y1 = p.y;
    tft.drawFastHLine(0, 10, 20, ILI9341_BLACK);
    tft.drawFastVLine(10, 0, 20, ILI9341_BLACK);
    delay(500);

    while (touchscreen.touched())
        ;
    tft.drawFastHLine(300, 230, 20, ILI9341_WHITE);
    tft.drawFastVLine(310, 220, 20, ILI9341_WHITE);

    while (!touchscreen.touched())
        ;
    delay(50);
    p = touchscreen.getPoint();
    x2 = p.x;
    y2 = p.y;
    tft.drawFastHLine(300, 230, 20, ILI9341_BLACK);
    tft.drawFastVLine(310, 220, 20, ILI9341_BLACK);

    int16_t xDist = 320 - 40;
    int16_t yDist = 240 - 40;

    global_config.screenCalXMult = (float)xDist / (float)(x2 - x1);
    global_config.screenCalXOffset = 20.0 - ((float)x1 * global_config.screenCalXMult);

    global_config.screenCalYMult = (float)yDist / (float)(y2 - y1);
    global_config.screenCalYOffset = 20.0 - ((float)y1 * global_config.screenCalYMult);

    global_config.screenCalibrated = true;
    WriteGlobalConfig();
}

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
    if (touchscreen.tirqTouched() && touchscreen.touched())
    {
        // dont pass first touch after power on
        if (is_screen_asleep())
        {
            screen_timer_wake();
            while (touchscreen.touched())
                ;
            return;
        }

        screen_timer_wake();

        TS_Point p = touchscreen_point();
        data->state = LV_INDEV_STATE_PR;
        data->point.x = p.x;
        data->point.y = p.y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void set_invert_display(){
    tft.invertDisplay(global_config.invertColors);
}

void screen_setup()
{
    touchscreen_spi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreen_spi);
    touchscreen.setRotation(global_config.rotateScreen ? 3 : 1);

    lv_init();

    tft.init();

    ledcSetup(0, 5000, 12);
    ledcAttachPin(21, 0);

    tft.setRotation(global_config.rotateScreen ? 3 : 1);
    tft.fillScreen(TFT_BLACK);
    set_screen_brightness();
    set_invert_display();

    touchscreen_spi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreen_spi);

    touchscreen_calibrate(false);

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

#endif // CYD_SCREEN_DRIVER_ESP32_2432S028R