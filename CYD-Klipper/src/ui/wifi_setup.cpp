#include "lvgl.h"
#include "wifi_setup.h"
#include "../conf/global_config.h"
#include "ui_utils.h"
#include "WiFi.h"

void WifiInitInner();

static void ResetBtnEventHandler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        globalConfig.wifiConfigured = false;
        WifiInitInner();
    }
}

static void RefreshBtnEventHandler(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        WifiInitInner();
    }
}

static void TaEventCb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (code == LV_EVENT_READY) 
    {
        const char * txt = lv_textarea_get_text(ta);
        int len = strlen(txt);
        if (len > 0)
        {
            globalConfig.wifiConfigured = true;
            strcpy(globalConfig.wifiPassword, txt);
            WriteGlobalConfig();
            WifiInitInner();
        }
    }
    else if (code == LV_EVENT_CANCEL)
    {
        WifiInitInner();
    }
}

void WifiPassEntry(const char* ssid){
    lv_obj_clean(lv_scr_act());

    lv_obj_t * root = CreateEmptyPanel(lv_scr_act());
    lv_obj_set_size(root, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    LayoutFlexColumn(root);

    lv_obj_t * top_root = CreateEmptyPanel(root);
    lv_obj_set_width(top_root, CYD_SCREEN_WIDTH_PX);
    LayoutFlexColumn(top_root);
    lv_obj_set_flex_grow(top_root, 1);
    lv_obj_set_style_pad_all(top_root, CYD_SCREEN_GAP_PX, 0);

    lv_obj_t * label = lv_label_create(top_root);
    lv_label_set_text(label, "Enter WiFi Password");
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);

    lv_obj_t * passEntry = lv_textarea_create(top_root);
    lv_textarea_set_one_line(passEntry, true);
    lv_textarea_set_text(passEntry, "");
    lv_obj_set_width(passEntry, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_add_event_cb(passEntry, TaEventCb, LV_EVENT_ALL, NULL);
    lv_obj_set_flex_grow(passEntry, 1);

    lv_obj_t * keyboard = lv_keyboard_create(root);
    lv_keyboard_set_textarea(keyboard, passEntry);
}

static void WifiBtnEventHandler(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        delay(100);
        char* ssid = (char*)e->user_data;
        strcpy(globalConfig.wifiSsid, ssid);
        Serial.println(ssid);
        WifiPassEntry(ssid);
    }
}

void WifiInitInner(){
    WiFi.disconnect();
    lv_obj_clean(lv_scr_act());

    if (globalConfig.wifiConfigured){
        WiFi.begin(globalConfig.wifiSsid, globalConfig.wifiPassword);

        lv_obj_t * label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "Connecting to WiFi");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t * resetBtn = lv_btn_create(lv_scr_act());
        lv_obj_add_event_cb(resetBtn, ResetBtnEventHandler, LV_EVENT_CLICKED, NULL);
        lv_obj_set_height(resetBtn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_obj_align(resetBtn, LV_ALIGN_CENTER, 0, CYD_SCREEN_GAP_PX + CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        label = lv_label_create(resetBtn);
        lv_label_set_text(label, "Reset");
        lv_obj_center(label);

        return;
    }

    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Scanning for networks...");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_timer_handler();
    lv_task_handler();
    lv_refr_now(NULL);

    lv_obj_clean(lv_scr_act());

    lv_obj_t * root = CreateEmptyPanel(lv_scr_act());
    lv_obj_set_size(root, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    LayoutFlexColumn(root);
    lv_obj_set_style_pad_all(root, CYD_SCREEN_GAP_PX, 0);

    lv_obj_t * top_row = CreateEmptyPanel(root);
    lv_obj_set_size(top_row, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, LV_SIZE_CONTENT);
    LayoutFlexRow(top_row, LV_FLEX_ALIGN_SPACE_BETWEEN);

    label = lv_label_create(top_row);
    lv_label_set_text(label, "Select a network");

    lv_obj_t * refreshBtn = lv_btn_create(top_row);
    lv_obj_add_event_cb(refreshBtn, ResetBtnEventHandler, LV_EVENT_ALL, NULL);
    lv_obj_set_size(refreshBtn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    label = lv_label_create(refreshBtn);
    lv_label_set_text(label, LV_SYMBOL_REFRESH);
    lv_obj_center(label);

    lv_obj_t * list = lv_list_create(root);
    lv_obj_set_width(list, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_set_flex_grow(list, 1);

    int n = WiFi.scanNetworks();

    for (int i = 0; i < n; ++i) {
        String ssid = WiFi.SSID(i);
        char* ssid_copy = (char*)malloc(ssid.length() + 1);
        int j = 0;

        for (; j < ssid.length(); ++j){
            if (ssid[j] == '\0')
                continue;

            ssid_copy[j] = ssid[j];
        }

        ssid_copy[j] = '\0';

        lv_obj_t * btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, ssid_copy);
        lv_obj_add_event_cb(btn, WifiBtnEventHandler, LV_EVENT_ALL, (void*)ssid_copy);
    }
}

const char* ERRS[] = {
    "Idle",
    "No SSID Available",
    "Scan Completed",
    "Connected",
    "Connection Failed",
    "Connection Lost",
    "Disconnected"
};

const int PRINT_FREQ = 1000;
int print_timer = 0;

void WifiInit(){
    WiFi.mode(WIFI_STA);
    WifiInitInner();

    while (!globalConfig.wifiConfigured || WiFi.status() != WL_CONNECTED){
        if (millis() - print_timer > PRINT_FREQ){
            print_timer = millis();
            Serial.printf("WiFi Status: %s\n", ERRS[WiFi.status()]);
        }

        lv_timer_handler();
        lv_task_handler();
    }
}

void WifiOk(){
    if (WiFi.status() != WL_CONNECTED){
        ESP.restart();
    }
}
