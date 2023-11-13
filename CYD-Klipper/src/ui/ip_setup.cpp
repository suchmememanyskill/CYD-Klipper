#include "ip_setup.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include <TFT_eSPI.h>
#include <HTTPClient.h>

bool connect_ok = false;
lv_obj_t * ipEntry;
lv_obj_t * portEntry;
lv_obj_t * label = NULL;

bool verify_ip(){
    HTTPClient client;
    String url = "http://" + String(global_config.klipperHost) + ":" + String(global_config.klipperPort) + "/printer/info";
    int httpCode;
    try {
        Serial.println(url);
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
        bool result = verify_ip();
        if (result)
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

void ip_setup_inner(){
    lv_obj_clean(lv_scr_act());

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

void ip_setup(){
    connect_ok = false;

    if (global_config.ipConfigured && verify_ip()){
        return;
    }

    ip_setup_inner();

    while (!connect_ok)
    {
        lv_timer_handler();
        lv_task_handler();
    }
}