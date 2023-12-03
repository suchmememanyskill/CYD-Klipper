#pragma once
// Adapted from https://github.com/xperiments-in/xtouch/blob/main/src/devices/2.8/screen.h

#ifndef _SCREEN_DRIVER_INIT
#define _SCREEN_DRIVER_INIT

#define CPU_FREQ_HIGH 240
#define CPU_FREQ_LOW 80

#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

TS_Point touchscreen_point();
void touchscreen_calibrate(bool force = false);
void screen_setBrightness(byte brightness);
void screen_timer_setup();
void screen_timer_start();
void screen_timer_stop();
void screen_timer_period(uint32_t period);
void set_color_scheme();
void screen_setup();
void set_invert_display();
void screen_timer_wake();
void set_screen_timer_period();
void set_screen_brightness();

extern TFT_eSPI tft;

#endif // _SCREEN_DRIVER_INIT