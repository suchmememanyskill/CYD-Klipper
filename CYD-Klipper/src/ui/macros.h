#pragma once

#include "lvgl.h"
#include "../core/macros_query.h"

void macros_add_macros_to_panel(lv_obj_t * root_panel, MACROSQUERY query);
void macros_add_power_devices_to_panel(lv_obj_t * root_panel, POWERQUERY query);
void macros_set_current_config(PRINTER_CONFIG * config);
void macros_draw_power_fullscreen(PRINTER_CONFIG * config);
void macros_draw_power_fullscreen();