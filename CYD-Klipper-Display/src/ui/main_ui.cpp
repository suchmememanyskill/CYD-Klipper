#include "main_ui.h"
#include "../core/data_setup.h"
#include "lvgl.h"
#include "nav_buttons.h"
#include <ArduinoJson.h>

char extruder_temp_buff[20];
char bed_temp_buff[20];
char position_buff[20];

void error_ui(){
    lv_obj_clean(lv_scr_act());
    
    lv_obj_t * label;
    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, LV_SYMBOL_WARNING " Printer is not ready");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, printer.state_message);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 30);
    lv_obj_set_size(label, TFT_HEIGHT - 20, TFT_WIDTH - 30);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
}

static void on_state_change(void * s, lv_msg_t * m){
    if (printer.state == PRINTER_STATE_ERROR){
        error_ui();
    }
    else {
        nav_buttons_setup(0);
    }
}


void main_ui_setup(){
    lv_msg_subscribe(DATA_PRINTER_STATE, on_state_change, NULL);
    on_state_change(NULL, NULL);
}