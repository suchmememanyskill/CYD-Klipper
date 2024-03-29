#ifdef CYD_SCREEN_DRIVER_ESP32_2432S028R
#include "../screen_driver.h"

#ifdef CYD_SCREEN_VERTICAL
    #error "Vertical screen not supported with the ESP32_2432S028R driver"
#endif

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
        TS_Point p = touchscreen.getPoint();
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
    tft.invertDisplay(get_current_printer_config()->invert_colors);
}

void screen_setup()
{
    touchscreen_spi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreen_spi);
    touchscreen.setRotation(global_config.rotate_screen ? 3 : 1);

    lv_init();

    tft.init();

    if (global_config.display_mode) {
        // <3 https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/cyd.md#the-display-doesnt-look-as-good
        tft.writecommand(ILI9341_GAMMASET); //Gamma curve selected
        tft.writedata(2);
        delay(120);
        tft.writecommand(ILI9341_GAMMASET); //Gamma curve selected
        tft.writedata(1);
    }

    ledcSetup(0, 5000, 12);
    ledcAttachPin(21, 0);

    tft.setRotation(global_config.rotate_screen ? 3 : 1);
    tft.fillScreen(TFT_BLACK);
    set_invert_display();
    touchscreen_spi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreen_spi);

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