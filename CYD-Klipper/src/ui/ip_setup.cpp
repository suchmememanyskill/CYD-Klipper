#include "ip_setup.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include "core/data_setup.h"

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

void ip_init_inner(){
    lv_obj_clean(lv_scr_act());

    if (global_config.ipConfigured) {
        label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "Connecting to Klipper");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t * resetBtn = lv_btn_create(lv_scr_act());
        lv_obj_add_event_cb(resetBtn, reset_btn_event_handler, LV_EVENT_ALL, NULL);
        lv_obj_align(resetBtn, LV_ALIGN_CENTER, 0, 40);

        lv_obj_t * btnLabel = lv_label_create(resetBtn);
        lv_label_set_text(btnLabel, "Reset");
        lv_obj_center(btnLabel);
        return;
    }

    lv_obj_t * keyboard = lv_keyboard_create(lv_scr_act());
    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Enter Klipper IP and Port");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10 + 2);

    ipEntry = lv_textarea_create(lv_scr_act());
    lv_textarea_set_one_line(ipEntry, true);
    lv_textarea_set_max_length(ipEntry, 63);
    lv_textarea_set_text(ipEntry, "");
    lv_obj_align(ipEntry, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_obj_add_event_cb(ipEntry, ta_event_cb, LV_EVENT_ALL, keyboard);
    lv_obj_set_size(ipEntry, TFT_HEIGHT - 20 - 100, 60);

    portEntry = lv_textarea_create(lv_scr_act());
    lv_textarea_set_one_line(portEntry, true);
    lv_textarea_set_max_length(portEntry, 5);
    lv_textarea_set_text(portEntry, "80");
    lv_obj_align(portEntry, LV_ALIGN_TOP_LEFT, TFT_HEIGHT - 20 - 80, 40);
    lv_obj_add_event_cb(portEntry, ta_event_cb, LV_EVENT_ALL, keyboard);
    lv_obj_set_size(portEntry, 90, 60);
    
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(keyboard, ipEntry);
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