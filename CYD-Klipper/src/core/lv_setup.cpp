#include "lv_setup.h"
#include "screen_driver.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include "../ui/ui_utils.h"
#include <Esp.h>

#ifndef CPU_FREQ_HIGH
#define CPU_FREQ_HIGH 240
#endif
#ifndef CPU_FREQ_LOW
#define CPU_FREQ_LOW 80
#endif

typedef void (*lv_indev_drv_read_cb_t)(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

bool is_screen_in_sleep = false;
lv_timer_t *screen_sleep_timer;
lv_coord_t point[2] = {0};

static lv_indev_drv_read_cb_t original_touch_driver = NULL;

void lv_touch_intercept_calibration(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) 
{
    original_touch_driver(indev_driver, data);

    if (data->state == LV_INDEV_STATE_PR){
        point[0] = data->point.x;
        point[1] = data->point.y;

        while (data->state == LV_INDEV_STATE_PR){
            original_touch_driver(indev_driver, data);
            delay(20);    
        }
    }

    data->state = LV_INDEV_STATE_REL;
}

void lv_touch_intercept(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) 
{
    original_touch_driver(indev_driver, data);
    
    if (data->state == LV_INDEV_STATE_PR) {
        if (is_screen_asleep()) {
            while (data->state == LV_INDEV_STATE_PR) {
                original_touch_driver(indev_driver, data);
                delay(20);
            }

            data->state = LV_INDEV_STATE_REL;
        }

        screen_timer_wake();
#ifndef CYD_SCREEN_DISABLE_TOUCH_CALIBRATION
        data->point.x = round((data->point.x * global_config.screenCalXMult) + global_config.screenCalXOffset);
        data->point.y = round((data->point.y * global_config.screenCalYMult) + global_config.screenCalYOffset);
#endif // CYD_SCREEN_DISABLE_TOUCH_CALIBRATION
    }
}

void lv_do_calibration(){
    if (global_config.screenCalibrated){
        return;
    }

    lv_indev_t * display_driver = lv_indev_get_next(NULL);
    display_driver->driver->read_cb = lv_touch_intercept_calibration;

    lv_obj_clean(lv_scr_act());
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Calibrate Screen");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * line_x = lv_line_create(lv_scr_act());
    lv_obj_t * line_y = lv_line_create(lv_scr_act());

    static lv_point_t line_points_x[] = { {0, 10}, {21, 10} };
    static lv_point_t line_points_y[] = { {10, 0}, {10, 21} };

    lv_line_set_points(line_x, line_points_x, 2);
    lv_obj_set_style_line_width(line_x, 1, 0);
    lv_line_set_points(line_y, line_points_y, 2);
    lv_obj_set_style_line_width(line_y, 1, 0);

#ifdef CYD_SCREEN_DRIVER_ESP32_SMARTDISPLAY
    lv_obj_align(line_x, LV_ALIGN_TOP_RIGHT, 1, 0);
    lv_obj_align(line_y, LV_ALIGN_TOP_RIGHT, -10, 0);        
#else
    lv_obj_align(line_x, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_align(line_y, LV_ALIGN_TOP_LEFT, 0, 0);
#endif
    
    while (true){
        lv_timer_handler();
        lv_task_handler();

        if (point[0] != 0 && point[1] != 0){
            break;
        }
    }

    lv_coord_t x1 = point[0];
    lv_coord_t y1 = point[1];
    point[0] = 0;
    point[1] = 0;

    lv_obj_del(line_x);
    lv_obj_del(line_y);

    line_x = lv_line_create(lv_scr_act());
    line_y = lv_line_create(lv_scr_act());
    lv_line_set_points(line_x, line_points_x, 2);
    lv_line_set_points(line_y, line_points_y, 2);
    lv_obj_set_style_line_width(line_x, 1, 0);
    lv_obj_set_style_line_width(line_y, 1, 0);

    
#ifdef CYD_SCREEN_DRIVER_ESP32_SMARTDISPLAY
    lv_obj_align(line_x, LV_ALIGN_BOTTOM_LEFT, 0, -10);
    lv_obj_align(line_y, LV_ALIGN_BOTTOM_LEFT, 0, 1);
#else
    lv_obj_align(line_x, LV_ALIGN_BOTTOM_RIGHT, 1, -10);
    lv_obj_align(line_y, LV_ALIGN_BOTTOM_RIGHT, -10, 1);
#endif

    while (true){
        lv_timer_handler();
        lv_task_handler();

        if (point[0] != 0 && point[1] != 0){
            break;
        }
    }

    lv_coord_t x2 = point[0];
    lv_coord_t y2 = point[1];

#ifdef CYD_SCREEN_DRIVER_ESP32_SMARTDISPLAY
    int16_t xDist = CYD_SCREEN_HEIGHT_PX - 20;
    int16_t yDist = CYD_SCREEN_WIDTH_PX - 20;
#else
    int16_t xDist = CYD_SCREEN_WIDTH_PX - 20;
    int16_t yDist = CYD_SCREEN_HEIGHT_PX - 20;
#endif

    global_config.screenCalXMult = (float)xDist / (float)(x2 - x1);
    global_config.screenCalXOffset = 10.0 - ((float)x1 * global_config.screenCalXMult);

    global_config.screenCalYMult = (float)yDist / (float)(y2 - y1);
    global_config.screenCalYOffset = 10.0 - ((float)y1 * global_config.screenCalYMult);

    if (global_config.screenCalXMult == std::numeric_limits<float>::infinity() || global_config.screenCalYMult == std::numeric_limits<float>::infinity()){
        Serial.println("Calibration failed, please try again");
        ESP.restart();
    }

    global_config.screenCalibrated = true;
    WriteGlobalConfig();

    lv_obj_clean(lv_scr_act());
    Serial.printf("Calibration done: X*%.2f + %.2f, Y*%.2f + %.2f\n", global_config.screenCalXMult, global_config.screenCalXOffset, global_config.screenCalYMult, global_config.screenCalYOffset);
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
#ifndef CYD_SCREEN_DISABLE_TIMEOUT
    lv_timer_reset(screen_sleep_timer);

    if (!is_screen_in_sleep){
        return;
    }

    is_screen_in_sleep = false;
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
    is_screen_in_sleep = true;

    // Screen is off, no need to make the cpu run fast, the user won't notice ;)
    setCpuFrequencyMhz(CPU_FREQ_LOW);
    Serial.printf("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz());
#endif
}

void screen_timer_setup()
{
    screen_sleep_timer = lv_timer_create(screen_timer_sleep, global_config.screenTimeout * 1000 * 60, NULL);
    lv_timer_pause(screen_sleep_timer);
}

void screen_timer_start()
{
    lv_timer_resume(screen_sleep_timer);
}

void screen_timer_stop()
{
    lv_timer_pause(screen_sleep_timer);
}

void screen_timer_period(unsigned int period)
{
    lv_timer_set_period(screen_sleep_timer, period);
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

void lv_setup()
{
    lv_indev_t * display_driver = lv_indev_get_next(NULL);

    if (original_touch_driver == NULL) 
    {
        original_touch_driver = display_driver->driver->read_cb;
    }

    set_color_scheme();

#ifndef CYD_SCREEN_DISABLE_TOUCH_CALIBRATION
    lv_do_calibration();
#endif // CYD_SCREEN_DISABLE_TOUCH_CALIBRATION

    display_driver->driver->read_cb = lv_touch_intercept;

    screen_timer_setup();
    screen_timer_start();
}

bool is_screen_asleep()
{
    return is_screen_in_sleep;
}