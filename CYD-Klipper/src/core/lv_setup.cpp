#include "lv_setup.h"
#include "screen_driver.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include "../ui/ui_utils.h"
#include <Esp.h>
#include "../ui/serial/serial_console.h"

#ifndef CPU_FREQ_HIGH
#define CPU_FREQ_HIGH 240
#endif
#ifndef CPU_FREQ_LOW
#define CPU_FREQ_LOW 80
#endif

unsigned long last_milis = 0;

void lv_handler()
{
#ifndef CYD_SCREEN_DISABLE_TOUCH_CALIBRATION
    if (digitalRead(0) == HIGH)
    {
        last_milis = millis();
    }
    else if (millis() - last_milis > 8000)
    {
        global_config.screen_calibrated = false;
        write_global_config();
        ESP.restart();
    }
#endif

    lv_timer_handler();
    lv_task_handler();
}

typedef void (*lv_indev_drv_read_cb_t)(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

bool is_screen_in_sleep = false;
lv_timer_t *screen_sleep_timer;
lv_coord_t point[2] = {0};

static lv_indev_drv_read_cb_t original_touch_driver = NULL;

void lv_touch_intercept_calibration(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) 
{
    original_touch_driver(indev_driver, data);

    if (data->state == LV_INDEV_STATE_PR){
        lv_coord_t local_point[] = {data->point.x, data->point.y};
        while (data->state == LV_INDEV_STATE_PR){
            original_touch_driver(indev_driver, data);
            delay(20);    
        }

        point[0] = local_point[0];
        point[1] = local_point[1];
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
        data->point.x = round((data->point.x * global_config.screen_cal_x_mult) + global_config.screen_cal_x_offset);
        data->point.y = round((data->point.y * global_config.screen_cal_y_mult) + global_config.screen_cal_y_offset);
#endif // CYD_SCREEN_DISABLE_TOUCH_CALIBRATION
    }
}

void lv_do_calibration(){
    if (global_config.screen_calibrated){
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
        lv_handler();
        serial_console::run();

        if (point[0] != 0 && point[1] != 0){
            break;
        }
    }

    delay(300);

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
        lv_handler();

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

    global_config.screen_cal_x_mult = (float)xDist / (float)(x2 - x1);
    global_config.screen_cal_x_offset = 10.0 - ((float)x1 * global_config.screen_cal_x_mult);

    global_config.screen_cal_y_mult = (float)yDist / (float)(y2 - y1);
    global_config.screen_cal_y_offset = 10.0 - ((float)y1 * global_config.screen_cal_y_mult);

    if (global_config.screen_cal_x_mult == std::numeric_limits<float>::infinity() || global_config.screen_cal_y_mult == std::numeric_limits<float>::infinity()){
        LOG_LN("Calibration failed, please try again");
        ESP.restart();
    }

    global_config.screen_calibrated = true;
    write_global_config();

    lv_obj_clean(lv_scr_act());
    LOG_F(("Calibration done: X*%.2f + %.2f, Y*%.2f + %.2f\n", global_config.screen_cal_x_mult, global_config.screen_cal_x_offset, global_config.screen_cal_y_mult, global_config.screen_cal_y_offset))
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
    LOG_F(("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz()))
#endif
}

void screen_timer_sleep(lv_timer_t *timer)
{
#ifndef CYD_SCREEN_DISABLE_TIMEOUT
    screen_setBrightness(0);
    is_screen_in_sleep = true;

    // Screen is off, no need to make the cpu run fast, the user won't notice ;)
    setCpuFrequencyMhz(CPU_FREQ_LOW);
    LOG_F(("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz()))
#endif
}

void screen_timer_setup()
{
    screen_sleep_timer = lv_timer_create(screen_timer_sleep, global_config.screen_timeout * 1000 * 60, NULL);
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
    screen_timer_period(global_config.screen_timeout * 1000 * 60);
}

void set_color_scheme()
{
    PRINTER_CONFIG *config = get_current_printer_config();
    lv_disp_t *dispp = lv_disp_get_default();
    lv_color_t main_color = {0};
    COLOR_DEF color_def = color_defs[config->color_scheme];

    if (color_defs[config->color_scheme].primary_color_light > 0){
        main_color = lv_palette_lighten(color_def.primary_color, color_def.primary_color_light);
    }
    else if (color_defs[config->color_scheme].primary_color_light < 0) {
        main_color = lv_palette_darken(color_def.primary_color, color_def.primary_color_light * -1);
    }
    else {
        main_color = lv_palette_main(color_defs[config->color_scheme].primary_color);
    }

    lv_theme_t *theme = lv_theme_default_init(dispp, main_color, lv_palette_main(color_def.secondary_color), !config->light_mode, &CYD_SCREEN_FONT);
    lv_disp_set_theme(dispp, theme);
}

void lv_setup()
{
    set_screen_brightness();

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
    lv_png_init();
}

bool is_screen_asleep()
{
    return is_screen_in_sleep;
}