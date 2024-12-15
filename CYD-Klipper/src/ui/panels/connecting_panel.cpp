#include "panel.h"
#include "../../core/printer_integration.hpp"

void connecting_panel_init(lv_obj_t* panel) 
{
    lv_obj_t* label = lv_label_create(panel);
    lv_label_set_text_fmt(label, "Connecting to %s...", (get_current_printer()->printer_config->printer_name[0] == 0) 
        ? get_current_printer()->printer_config->printer_host 
        : get_current_printer()->printer_config->printer_name);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}