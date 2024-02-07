#include "main_ui.h"
#include "../core/data_setup.h"
#include "../conf/global_config.h"
#include "../core/screen_driver.h"
#include "lvgl.h"
#include "nav_buttons.h"
#include "ui_utils.h"
#include "panels/panel.h"
#include "../core/macros_query.h"
#include "../core/lv_setup.h"

char extruder_temp_buff[20];
char bed_temp_buff[20];
char position_buff[20];

static void btn_click_restart(lv_event_t * e){
    send_gcode(false, "RESTART");
}

static void btn_click_firmware_restart(lv_event_t * e){
    send_gcode(false, "FIRMWARE_RESTART");
}

void error_ui_macros_open(lv_event_t * e){
    lv_obj_t * panel = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0); 
    lv_layout_flex_column(panel);
    lv_obj_set_size(panel, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, 0, CYD_SCREEN_GAP_PX);

    lv_obj_t * button = lv_btn_create(panel);
    lv_obj_set_size(button, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(button, destroy_event_user_data, LV_EVENT_CLICKED, panel);

    lv_obj_t * label = lv_label_create(button);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " Close");
    lv_obj_center(label);

    macros_panel_add_power_devices_to_panel(panel, power_devices_query());
}

void error_ui(){
    lv_obj_clean(lv_scr_act());

    lv_obj_t * panel = lv_create_empty_panel(lv_scr_act());
    lv_layout_flex_column(panel);
    lv_obj_set_size(panel, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    lv_obj_t * label;
    label = lv_label_create(panel);
    lv_label_set_text(label, LV_SYMBOL_WARNING " Printer is not ready");

    label = lv_label_create(panel);
    lv_label_set_text(label, printer.state_message);
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

    lv_obj_t * button_row = lv_create_empty_panel(panel);
    lv_obj_set_size(button_row, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_layout_flex_row(button_row);

    lv_obj_t * btn = lv_btn_create(button_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, btn_click_restart, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Restart");
    lv_obj_center(label);

    btn = lv_btn_create(button_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, btn_click_firmware_restart, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, "FW Restart");
    lv_obj_center(label);

    if (power_devices_query().count >= 1){
        btn = lv_btn_create(button_row);
        lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_obj_add_event_cb(btn, error_ui_macros_open, LV_EVENT_CLICKED, NULL);
        lv_obj_set_flex_grow(btn, 1);

        label = lv_label_create(btn);
        lv_label_set_text(label, "Devices");
        lv_obj_center(label);
    }
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