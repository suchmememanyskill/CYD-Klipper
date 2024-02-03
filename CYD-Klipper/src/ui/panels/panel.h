#include "lvgl.h"
#include "../../core/macros_query.h"

#define SIZEOF(arr) (sizeof(arr) / sizeof(*arr))

void settings_panel_init(lv_obj_t* panel);
void temp_panel_init(lv_obj_t* panel);
void print_panel_init(lv_obj_t* panel);
void move_panel_init(lv_obj_t* panel);
void progress_panel_init(lv_obj_t* panel);
void macros_panel_init(lv_obj_t* panel);
void stats_panel_init(lv_obj_t* panel);
void macros_panel_add_power_devices_to_panel(lv_obj_t * panel, POWERQUERY query);