#include "ESP32-2432S028R.h"

#include <SPI.h>
#include <TFT_eSPI.h>
#include "../../conf/global_config.h"
#include "lvgl.h"
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define CPU_FREQ_HIGH 240
#define CPU_FREQ_LOW 80

SPIClass touchscreen_spi = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

uint32_t LV_EVENT_GET_COMP_CHILD;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[TFT_WIDTH * TFT_HEIGHT / 10];

TFT_eSPI tft = TFT_eSPI();

bool isScreenInSleep = false;
lv_timer_t *screenSleepTimer;

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

void set_screen_brightness()
{
    if (global_config.brightness < 32)
        screen_setBrightness(255);
    else
        screen_setBrightness(global_config.brightness);
}

void screen_timer_wake()
{
    lv_timer_reset(screenSleepTimer);
    isScreenInSleep = false;
    set_screen_brightness();

    // Reset cpu freq
    setCpuFrequencyMhz(CPU_FREQ_HIGH);
    Serial.printf("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz());
}

void screen_timer_sleep(lv_timer_t *timer)
{
    screen_setBrightness(0);
    isScreenInSleep = true;

    // Screen is off, no need to make the cpu run fast, the user won't notice ;)
    setCpuFrequencyMhz(CPU_FREQ_LOW);
    Serial.printf("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz());
}

void screen_timer_setup()
{
    screenSleepTimer = lv_timer_create(screen_timer_sleep, global_config.screenTimeout * 1000 * 60, NULL);
    lv_timer_pause(screenSleepTimer);
}

void screen_timer_start()
{
    lv_timer_resume(screenSleepTimer);
}

void screen_timer_stop()
{
    lv_timer_pause(screenSleepTimer);
}

void screen_timer_period(uint32_t period)
{
    lv_timer_set_period(screenSleepTimer, period);
}

void set_screen_timer_period()
{
    screen_timer_period(global_config.screenTimeout * 1000 * 60);
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
        lv_timer_reset(screenSleepTimer);
        // dont pass first touch after power on
        if (isScreenInSleep)
        {
            screen_timer_wake();
            while (touchscreen.touched())
                ;
            return;
        }

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

void set_color_scheme(){
    lv_disp_t *dispp = lv_disp_get_default();
    lv_color_t main_color = {0};
    COLOR_DEF color_def = color_defs[global_config.color_scheme];

    if (color_defs[global_config.color_scheme].primary_color_light > 0){
        main_color = lv_palette_lighten(color_def.primary_color, color_def.primary_color_light);
    }
    else if (color_defs[global_config.color_scheme].primary_color_light < 0) {
        main_color = lv_palette_darken(color_def.primary_color, color_def.primary_color_light * -1);
    }
    else {
        main_color = lv_palette_main(color_defs[global_config.color_scheme].primary_color);
    }

    lv_theme_t *theme = lv_theme_default_init(dispp, main_color, lv_palette_main(color_def.secondary_color), !global_config.lightMode, CYD_SCREEN_FONT);
    lv_disp_set_theme(dispp, theme);
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

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_WIDTH * TFT_HEIGHT / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_HEIGHT;
    disp_drv.ver_res = TFT_WIDTH;
    disp_drv.flush_cb = screen_lv_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);


    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = screen_lv_touchRead;
    lv_indev_drv_register(&indev_drv);

    screen_timer_setup();
    screen_timer_start();

    /*Initialize the graphics library */
    LV_EVENT_GET_COMP_CHILD = lv_event_register_id();
    set_color_scheme();
}