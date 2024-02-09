
#ifdef CYD_SCREEN_DRIVER_ESP32_SMARTDISPLAY

#include "../screen_driver.h"
#include <esp32_smartdisplay.h>
#include "../../conf/global_config.h"
#include "lvgl.h"
#include "../lv_setup.h"

void touchscreen_calibrate(bool force)
{
    // TODO: Stubbed
    return;
}

void screen_setBrightness(byte brightness)
{
    smartdisplay_lcd_set_backlight(brightness / 255.0f);
}

void set_invert_display(){
    // Stubbed
}

void screen_setup()
{
    smartdisplay_init();

    lv_disp_set_rotation(lv_disp_get_default(), (global_config.rotateScreen) ? LV_DISP_ROT_270 : LV_DISP_ROT_90);
}

#endif // CYD_SCREEN_DRIVER_ESP32_SMARTDISPLAY