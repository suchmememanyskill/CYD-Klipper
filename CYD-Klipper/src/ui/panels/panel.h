#include "lvgl.h"
#include "../../core/macros_query.h"

#define SIZEOF(arr) (sizeof(arr) / sizeof(*arr))

void settings_panel_init(lv_obj_t* panel);
void temp_panel_init(lv_obj_t* panel);
void files_panel_init(lv_obj_t* panel);
void move_panel_init(lv_obj_t* panel);
void progress_panel_init(lv_obj_t* panel);
void macros_panel_init(lv_obj_t* panel);
void stats_panel_init(lv_obj_t* panel);
void printer_panel_init(lv_obj_t* panel);
void error_panel_init(lv_obj_t* panel);
void connecting_panel_init(lv_obj_t* panel);