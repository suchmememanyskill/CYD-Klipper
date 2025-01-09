#pragma once

#include "lvgl.h"
#include "../conf/global_config.h"
#include "../core/printer_integration.hpp"

int macros_add_macros_to_panel(lv_obj_t * root_panel, BasePrinter* printer);
int macros_add_power_devices_to_panel(lv_obj_t * root_panel, BasePrinter* printer);
void macros_draw_power_fullscreen(BasePrinter* printer);
void macros_draw_power_fullscreen();