#ifdef CYD_BOARD_GUITION_35

#include "../screen_driver.h"
#include <databus/Arduino_ESP32QSPI.h>
#include <display/Arduino_AXS15231B.h>
#include <canvas/Arduino_Canvas.h>
#include "lvgl.h"
#include "../lv_setup.h"
#include "../../conf/global_config.h"

#define CPU_FREQ_HIGH 240
#define CPU_FREQ_LOW 80

#define CANVAS_WIDTH CYD_SCREEN_WIDTH_PX
#define CANVAS_HEIGHT CYD_SCREEN_HEIGHT_PX


static Arduino_ESP32QSPI qspiBus(
    LCD_CS, LCD_CLK,
    LCD_D0, LCD_D1,
    LCD_D2, LCD_D3,
    false);

Arduino_GFX *gfx = new Arduino_AXS15231B(
    &qspiBus, -1, 0, true,
    LCD_WIDTH, LCD_HEIGHT,
    0, 0, 0, 0);
static Arduino_Canvas canvas(CANVAS_WIDTH, CANVAS_HEIGHT, gfx);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf = nullptr;

void screen_setBrightness(uint8_t brightness)
{
    uint32_t duty = (4095UL * brightness) / 255UL;
    ledcWrite(0, duty);
}

void screen_lv_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    // Full frame flush each time
    canvas.draw16bitRGBBitmap(0, 0, (uint16_t *)color_p, CANVAS_WIDTH, CANVAS_HEIGHT);
    canvas.flush();
    lv_disp_flush_ready(disp);
}

void screen_lv_touchRead(lv_indev_drv_t * /*indev_driver*/, lv_indev_data_t *data)
{
    data->state = LV_INDEV_STATE_REL;  // No touch support
}

void set_invert_display()
{
    gfx->invertDisplay(global_config.printer_config[global_config.printer_index].invert_colors);
}

void screen_setup()
{
    pinMode(LCD_BL_PIN, OUTPUT);
    ledcSetup(0, 5000, 12);
    ledcAttachPin(LCD_BL_PIN, 0);
    screen_setBrightness(255);

    // gfx->begin();
    canvas.begin();
    gfx->invertDisplay(true);  // OK after begin()
    canvas.fillScreen(0x0000);
    canvas.flush();

    lv_init();

    // Allocate full canvas buffer for LVGL
    buf = (lv_color_t *)heap_caps_malloc(
        CANVAS_WIDTH * CANVAS_HEIGHT * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    assert(buf);

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, CANVAS_WIDTH * CANVAS_HEIGHT);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = CANVAS_WIDTH;
    disp_drv.ver_res = CANVAS_HEIGHT;
    disp_drv.flush_cb = screen_lv_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = screen_lv_touchRead;
    lv_indev_drv_register(&indev_drv);
}

#endif // CYD_BOARD_GUITION_35
