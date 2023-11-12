#include "lvgl.h"
#include "panel.h"
#include "../../core/data_setup.h"

void print_panel_init(lv_obj_t* panel){
    if (printer.state == PRINTER_STATE_PRINTING || printer.state == PRINTER_STATE_PAUSED){
        progress_panel_init(panel);
        return;
    }
}