#pragma once
// Adapted from https://github.com/xperiments-in/xtouch/blob/main/src/devices/2.8/screen.h

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