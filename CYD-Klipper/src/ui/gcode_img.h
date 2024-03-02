#pragma once
#include "lvgl.h"

lv_obj_t* show_gcode_img(const char* filename);
bool has_128_128_gcode(const char* filename);
void clear_img_mem();