
#ifdef CYD_SCREEN_DRIVER_ESP32_SMARTDISPLAY

#include "../screen_driver.h"
#include <esp32_smartdisplay.h>
#include "../../conf/global_config.h"
#include "lvgl.h"
#include "../lv_setup.h"

typedef void (*lv_disp_drv_t_flush_cb)(_lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
static lv_disp_drv_t_flush_cb original_screen_driver;

void screen_setBrightness(byte brightness)
{
    smartdisplay_lcd_set_backlight(brightness / 255.0f);
}

void set_invert_display(){
    lv_obj_invalidate(lv_scr_act());
}

void lv_screen_intercept(_lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    if (global_config.invertColors) {
        uint32_t w = (area->x2 - area->x1 + 1);
        uint32_t h = (area->y2 - area->y1 + 1);

        for (int i = 0 ; i < w * h; i++){
            color_p[i].full ^= 0xFFFF;
        }
    }

    original_screen_driver(disp_drv, area, color_p);
}

void screen_setup()
{
    smartdisplay_init();

#ifndef CYD_SCREEN_DISABLE_INVERT_COLORS
    if (original_screen_driver == NULL){
        original_screen_driver = lv_disp_get_default()->driver->flush_cb;
        lv_disp_get_default()->driver->flush_cb = lv_screen_intercept;
    }
#endif // CYD_SCREEN_DISABLE_INVERT_COLORS

#ifdef CYD_SCREEN_IS_PORTRAIT
    lv_disp_set_rotation(lv_disp_get_default(), (global_config.rotateScreen) ? LV_DISP_ROT_180 : LV_DISP_ROT_NONE);
#else 
    lv_disp_set_rotation(lv_disp_get_default(), (global_config.rotateScreen) ? LV_DISP_ROT_270 : LV_DISP_ROT_90);
#endif // CYD_SCREEN_IS_PORTRAIT
}

#endif // CYD_SCREEN_DRIVER_ESP32_SMARTDISPLAY