#include "ip_setup.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include "core/data_setup.h"
#include "ui_utils.h"
#include "../core/macros_query.h"
#include "panels/panel.h"

bool connect_ok = false;
lv_obj_t * ipEntry;
lv_obj_t * portEntry;
lv_obj_t * label = NULL;

void ip_init_inner();

bool verify_ip(){
    HTTPClient client;
    String url = "http://" + String(global_config.klipperHost) + ":" + String(global_config.klipperPort) + "/printer/info";
    int httpCode;
    try {
        Serial.println(url);
        client.setTimeout(500);
        client.begin(url.c_str());
        httpCode = client.GET();
        return httpCode == 200;
    }
    catch (...) {
        Serial.println("Failed to connect");
        return false;
    }
}

static void ta_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if (code == LV_EVENT_READY) 
    {
        strcpy(global_config.klipperHost, lv_textarea_get_text(ipEntry));
        global_config.klipperPort = atoi(lv_textarea_get_text(portEntry));

        if (verify_ip())
        {
            global_config.ipConfigured = true;
            WriteGlobalConfig();
            connect_ok = true;
        }
        else
        {
            lv_label_set_text(label, "Failed to connect");
        }
    }
}

static void reset_btn_event_handler(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        global_config.ipConfigured = false;
        ip_init_inner();
    }
}

static void power_devices_button(lv_event_t * e) {
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

void redraw_connect_screen(){
    lv_obj_clean(lv_scr_act());

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Connecting to Klipper");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * button_row = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(button_row, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_layout_flex_row(button_row, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(button_row, LV_ALIGN_CENTER, 0, CYD_SCREEN_GAP_PX + CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t * reset_btn = lv_btn_create(button_row);
    lv_obj_add_event_cb(reset_btn, reset_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_height(reset_btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t * btn_label = lv_label_create(reset_btn);
    lv_label_set_text(btn_label, "Reset");
    lv_obj_center(btn_label);

    if (power_devices_query().count >= 1){
        lv_obj_t * power_devices_btn = lv_btn_create(button_row);
        lv_obj_add_event_cb(power_devices_btn, power_devices_button, LV_EVENT_CLICKED, NULL);
        lv_obj_set_height(power_devices_btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        btn_label = lv_label_create(power_devices_btn);
        lv_label_set_text(btn_label, "Power Devices");
        lv_obj_center(btn_label);
    }
}

void ip_init_inner(){
    if (global_config.ipConfigured) {
        redraw_connect_screen();
        return;
    }

    lv_obj_clean(lv_scr_act());

    lv_obj_t * root = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(root, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_layout_flex_column(root);

    lv_obj_t * top_root = lv_create_empty_panel(root);
    lv_obj_set_width(top_root, CYD_SCREEN_WIDTH_PX);
    lv_layout_flex_column(top_root);
    lv_obj_set_flex_grow(top_root, 1);
    lv_obj_set_style_pad_all(top_root, CYD_SCREEN_GAP_PX, 0);

    label = lv_label_create(top_root);
    lv_label_set_text(label, "Enter Klipper IP and Port");
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);

    lv_obj_t * textbow_row = lv_create_empty_panel(top_root);
    lv_obj_set_width(textbow_row, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_set_flex_grow(textbow_row, 1);
    lv_layout_flex_row(textbow_row);

    ipEntry = lv_textarea_create(textbow_row);
    lv_textarea_set_one_line(ipEntry, true);
    lv_textarea_set_max_length(ipEntry, 63);
    lv_textarea_set_text(ipEntry, "");
    lv_obj_set_flex_grow(ipEntry, 3);

    portEntry = lv_textarea_create(textbow_row);
    lv_textarea_set_one_line(portEntry, true);
    lv_textarea_set_max_length(portEntry, 5);
    lv_textarea_set_text(portEntry, "80");
    lv_obj_set_flex_grow(portEntry, 1);
    
    lv_obj_t * keyboard = lv_keyboard_create(root);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(keyboard, ipEntry);
    lv_obj_add_event_cb(ipEntry, ta_event_cb, LV_EVENT_ALL, keyboard);
    lv_obj_add_event_cb(portEntry, ta_event_cb, LV_EVENT_ALL, keyboard);
}

long last_data_update_ip = -10000;
const long data_update_interval_ip = 10000;
int retry_count = 0;

void ip_init(){
    connect_ok = false;
    retry_count = 0;

    ip_init_inner();

    while (!connect_ok)
    {
        lv_timer_handler();
        lv_task_handler();

        if (!connect_ok && global_config.ipConfigured && (millis() - last_data_update_ip) > data_update_interval_ip){
            connect_ok = verify_ip();
            last_data_update_ip = millis();
            retry_count++;
            String retry_count_text = "Connecting to Klipper (Try " + String(retry_count + 1) + ")";
            lv_label_set_text(label, retry_count_text.c_str());

            _power_devices_query_internal();
            if (power_devices_query().count >= 1)
                redraw_connect_screen();
        }
    }
}

void ip_ok(){
    if (klipper_request_consecutive_fail_count > 5){
        freeze_request_thread();
        ip_init();
        unfreeze_request_thread();
        klipper_request_consecutive_fail_count = 0;
        lv_msg_send(DATA_PRINTER_STATE, &printer);
    }
}