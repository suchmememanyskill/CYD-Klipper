#include "main_ui.h"
#include "../core/data_setup.h"
#include "lvgl.h"
#include "nav_buttons.h"
#include <ArduinoJson.h>

char extruder_temp_buff[20];
char bed_temp_buff[20];
char position_buff[20];
/*
static void update_printer_state(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);

    lv_label_set_text(label, printer_state_messages[printer.state]);
}

static void update_printer_state_message(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);

    lv_label_set_text(label, printer.state_message);
}

static void update_printer_data_temp(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);

    sprintf(extruder_temp_buff, "E: %.1f/%.1f", printer.extruder_temp, printer.extruder_target_temp);
    lv_label_set_text(label, extruder_temp_buff);
}

static void update_printer_data_bed_temp(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);

    sprintf(bed_temp_buff, "B: %.1f/%.1f", printer.bed_temp, printer.bed_target_temp);
    lv_label_set_text(label, bed_temp_buff);
}

static void update_printer_data_position(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);

    sprintf(position_buff, "P: %.0f/%.0f/%.0f", printer.position[0], printer.position[1], printer.position[2]);
    lv_label_set_text(label, position_buff);
}

void main_ui(){
    lv_obj_clean(lv_scr_act());
    
    lv_obj_t * label;
    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Waiting for update...");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(label, update_printer_state, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_STATE, label, NULL);

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Waiting for update...");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_add_event_cb(label, update_printer_state_message, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_STATE, label, NULL);

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Waiting for update...");
    lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_event_cb(label, update_printer_data_temp, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Waiting for update...");
    lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 100, 0);
    lv_obj_add_event_cb(label, update_printer_data_bed_temp, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Waiting for update...");
    lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 200, 0);
    lv_obj_add_event_cb(label, update_printer_data_position, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);
}
*/

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