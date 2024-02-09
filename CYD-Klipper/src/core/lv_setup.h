#pragma once

#include "lvgl.h"

typedef void (*lv_indev_drv_read_cb_t)(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

void set_screen_brightness();
void set_screen_timer_period();
void screen_timer_wake();
void screen_timer_start();
void screen_timer_stop();
void set_color_scheme();
void lv_setup();
bool is_screen_asleep();

lv_style_t * get_default_label_style();