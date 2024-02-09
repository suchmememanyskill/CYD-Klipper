#pragma once
// Adapted from https://github.com/xperiments-in/xtouch/blob/main/src/devices/2.8/screen.h

void touchscreen_calibrate(bool force = false);
void screen_setBrightness(unsigned char brightness);
void screen_setup();
void set_invert_display();