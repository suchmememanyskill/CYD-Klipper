#include "lvgl.h"
#include "wifi_setup.h"
#include "../conf/global_config.h"

#include "WiFi.h"
void wifi_init_inner();

static void reset_btn_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        global_config.wifiConfigured = false;
        wifi_init_inner();
    }
}

static void refresh_btn_event_handler(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        wifi_init_inner();
    }
}

static void ta_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (code == LV_EVENT_READY) 
    {
        const char * txt = lv_textarea_get_text(ta);
        int len = strlen(txt);
        if (len > 0)
        {
            global_config.wifiConfigured = true;
            strcpy(global_config.wifiPassword, txt);
            WriteGlobalConfig();
            wifi_init_inner();
        }
    }
    else if (code == LV_EVENT_CANCEL)
    {
        wifi_init_inner();
    }
}

void wifi_pass_entry(const char* ssid){
    lv_obj_clean(lv_scr_act());

    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Enter WiFi Password");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10 + 2);

    lv_obj_t * passEntry = lv_textarea_create(lv_scr_act());
    lv_textarea_set_one_line(passEntry, true);
    lv_textarea_set_text(passEntry, "");
    lv_obj_align(passEntry, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_obj_add_event_cb(passEntry, ta_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_size(passEntry, TFT_HEIGHT - 20, 60);

    lv_obj_t * keyboard = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_textarea(keyboard, passEntry);
}

static void wifi_btn_event_handler(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        delay(100);
        char* ssid = (char*)e->user_data;
        strcpy(global_config.wifiSSID, ssid);
        Serial.println(ssid);
        wifi_pass_entry(ssid);
    }
}


void wifi_init_inner(){
    WiFi.disconnect();

    if (global_config.wifiConfigured){
        WiFi.begin(global_config.wifiSSID, global_config.wifiPassword);
        
        lv_obj_clean(lv_scr_act());

        lv_obj_t * label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "Connecting to WiFi");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t * resetBtn = lv_btn_create(lv_scr_act());
        lv_obj_add_event_cb(resetBtn, reset_btn_event_handler, LV_EVENT_ALL, NULL);
        lv_obj_align(resetBtn, LV_ALIGN_CENTER, 0, 40);

        label = lv_label_create(resetBtn);
        lv_label_set_text(label, "Reset");
        lv_obj_center(label);

        return;
    } 

    lv_obj_clean(lv_scr_act());

    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Scanning for networks...");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_timer_handler();
    lv_task_handler();
    lv_refr_now(NULL);

    int n = WiFi.scanNetworks();

    lv_obj_clean(lv_scr_act());

    lv_obj_t * refreshBtn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(refreshBtn, reset_btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(refreshBtn, LV_ALIGN_TOP_RIGHT, -5, 5 - 1);

    label = lv_label_create(refreshBtn);
    lv_label_set_text(label, LV_SYMBOL_REFRESH);
    lv_obj_center(label);

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Select a network");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10 + 2);

    lv_obj_t * list = lv_list_create(lv_scr_act());
    lv_obj_align(list, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_obj_set_size(list, TFT_HEIGHT - 20, TFT_WIDTH - 40 - 5);

    for (int i = 0; i < n; ++i) {
        const char* ssid = WiFi.SSID(i).c_str();
        int len = strlen(ssid);

        if (len == 0)
            continue;

        const char* ssid_copy = (const char*)malloc(len + 1);
        strcpy((char*)ssid_copy, ssid);
        lv_obj_t * btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, ssid_copy);
        lv_obj_add_event_cb(btn, wifi_btn_event_handler, LV_EVENT_ALL, (void*)ssid_copy);
    }
}

void wifi_init(){
    WiFi.mode(WIFI_STA);
    wifi_init_inner();

    while (!global_config.wifiConfigured || WiFi.status() != WL_CONNECTED){
        lv_timer_handler();
        lv_task_handler();
    }
}

void wifi_ok(){
    if (WiFi.status() != WL_CONNECTED){
        wifi_init();
    }
}