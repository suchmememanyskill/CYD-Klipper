#pragma once
// Adapted from https://github.com/xperiments-in/xtouch/blob/main/src/devices/2.8/screen.h

#ifdef CYD_SCREEN_DRIVER_ESP32_2432S028R
    #include "device/ESP32-2432S028R.h"
#else
    #error "No screen driver defined"
#endif

void touchscreen_calibrate(bool force = false);
void screen_setBrightness(unsigned char brightness);
void screen_timer_setup();
void screen_timer_start();
void screen_timer_stop();
void screen_timer_period(unsigned int period);
void set_color_scheme();
void screen_setup();
void set_invert_display();
void screen_timer_wake();
void set_screen_timer_period();
void set_screen_brightness();