#include "main_ui.h"
#include "../core/data_setup.h"
#include "../conf/global_config.h"
#include "../core/screen_driver.h"
#include "lvgl.h"
#include "nav_buttons.h"

char extruder_temp_buff[20];
char bed_temp_buff[20];
char position_buff[20];

static void btn_click_restart(lv_event_t * e){
    send_gcode(false, "RESTART");
}

static void btn_click_firmware_restart(lv_event_t * e){
    send_gcode(false, "FIRMWARE_RESTART");
}

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

    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_size(btn, TFT_HEIGHT / 2 - 15, 30);
    lv_obj_add_event_cb(btn, btn_click_restart, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Restart");
    lv_obj_center(label);

    btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_size(btn, TFT_HEIGHT / 2 - 15, 30);
    lv_obj_add_event_cb(btn, btn_click_firmware_restart, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Firmware Restart");
    lv_obj_center(label);
}

void check_if_screen_needs_to_be_disabled(){
    if (global_config.onDuringPrint && printer.state == PRINTER_STATE_PRINTING){
        screen_timer_wake();
        screen_timer_stop();
    }
    else {
        screen_timer_start();
    }  
}

static void on_state_change(void * s, lv_msg_t * m){
    check_if_screen_needs_to_be_disabled();
    
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