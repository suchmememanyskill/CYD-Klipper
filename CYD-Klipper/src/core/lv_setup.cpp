#include "lv_setup.h"
#include "screen_driver.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include <Esp.h>

#ifndef CPU_FREQ_HIGH
#define CPU_FREQ_HIGH 240
#endif
#ifndef CPU_FREQ_LOW
#define CPU_FREQ_LOW 80
#endif

typedef void (*lv_indev_drv_read_cb_t)(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

bool isScreenInSleep = false;
lv_timer_t *screenSleepTimer;

void set_screen_brightness()
{
    if (global_config.brightness < 32)
        screen_setBrightness(255);
    else
        screen_setBrightness(global_config.brightness);
}

void screen_timer_wake()
{
#ifndef CYD_SCREEN_DISABLE_TIMEOUT
    lv_timer_reset(screenSleepTimer);

    if (!isScreenInSleep){
        return;
    }

    isScreenInSleep = false;
    set_screen_brightness();

    // Reset cpu freq
    setCpuFrequencyMhz(CPU_FREQ_HIGH);
    Serial.printf("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz());
#endif
}

void screen_timer_sleep(lv_timer_t *timer)
{
#ifndef CYD_SCREEN_DISABLE_TIMEOUT
    screen_setBrightness(0);
    isScreenInSleep = true;

    // Screen is off, no need to make the cpu run fast, the user won't notice ;)
    setCpuFrequencyMhz(CPU_FREQ_LOW);
    Serial.printf("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz());
#endif
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

void screen_timer_period(unsigned int period)
{
    lv_timer_set_period(screenSleepTimer, period);
}

void set_screen_timer_period()
{
    screen_timer_period(global_config.screenTimeout * 1000 * 60);
}

void set_color_scheme()
{
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

    lv_theme_t *theme = lv_theme_default_init(dispp, main_color, lv_palette_main(color_def.secondary_color), !global_config.lightMode, &CYD_SCREEN_FONT);
    lv_disp_set_theme(dispp, theme);
}

static lv_indev_drv_read_cb_t original_driver = NULL;

void lv_touch_intercept(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) 
{
    original_driver(indev_driver, data);

    if (data->state == LV_INDEV_STATE_PR) {
        if (is_screen_asleep()) {
            while (data->state == LV_INDEV_STATE_PR) {
                original_driver(indev_driver, data);
                delay(20);
            }

            data->state = LV_INDEV_STATE_REL;
        }

        screen_timer_wake();
    }
}

void lv_setup()
{
    screen_timer_setup();
    screen_timer_start();
    set_color_scheme();

    if (original_driver != NULL)
        return;

    lv_indev_t * display_driver = lv_indev_get_next(NULL);
    original_driver = display_driver->driver->read_cb;
    display_driver->driver->read_cb = lv_touch_intercept;
}

bool is_screen_asleep()
{
    return isScreenInSleep;
}