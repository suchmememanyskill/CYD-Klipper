#pragma once
// Adapted from https://github.com/xperiments-in/xtouch/blob/main/src/devices/2.8/screen.h

#ifndef _SCREEN_DRIVER_INIT
#define _SCREEN_DRIVER_INIT

#include <XPT2046_Touchscreen.h>

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
void screen_setup();

#endif // _SCREEN_DRIVER_INIT