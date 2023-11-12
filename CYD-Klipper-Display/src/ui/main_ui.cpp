#include "main_ui.h"
#include "../core/data_setup.h"
#include "lvgl.h"
#include <ArduinoJson.h>

char extruder_temp_buff[20];
char bed_temp_buff[20];
char position_buff[20];

static void update_printer_state(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);

    lv_label_set_text(label, printer_state_messages[printer.state]);
}

static void update_printer_state_message(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);

    lv_label_set_text(label, printer.state_message);
}

static void update_printer_data_extruder_temp(lv_event_t * e) {
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
    lv_obj_add_event_cb(label, update_printer_data_extruder_temp, LV_EVENT_MSG_RECEIVED, NULL);
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

void main_ui_setup(){
    // TODO: Subscribe to events
    main_ui();
    lv_msg_send(DATA_PRINTER_STATE, &printer);
    lv_msg_send(DATA_PRINTER_DATA, &printer);
}