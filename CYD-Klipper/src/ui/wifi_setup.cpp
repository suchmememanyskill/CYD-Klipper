#include "lvgl.h"
#include "wifi_setup.h"
#include "../conf/global_config.h"
#include "ui_utils.h"
#include "WiFi.h"

void wifi_init_inner();
void wifi_pass_entry(const char* ssid);

const char * current_ssid_ptr = NULL;

static void reset_btn_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        global_config.wifi_configured = false;
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

            global_config.wifi_configured = true;
            strcpy(global_config.wifi_SSID, current_ssid_ptr);
            strcpy(global_config.wifi_password, txt);
            write_global_config();
            wifi_init_inner();
        }
    }
    else if (code == LV_EVENT_CANCEL)
    {
        wifi_init_inner();
    }
}

static void btn_reuse_password(lv_event_t * e)
{
    ESP.restart();
}

static void btn_no_reuse_password(lv_event_t * e)
{
    const char * ssid = (const char *)lv_event_get_user_data(e);
    global_config.wifi_SSID[0] = '\0';
    wifi_pass_entry(ssid);
}

void ask_reuse_password(const char * ssid){
    lv_obj_clean(lv_scr_act());

    lv_obj_t * root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(root, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_align(root, LV_ALIGN_CENTER, 0, 0);
    lv_layout_flex_column(root, LV_FLEX_ALIGN_SPACE_BETWEEN);

    lv_obj_t * label = lv_label_create(root);
    lv_label_set_text_fmt(label, "Reuse stored WiFi Password?\n(Password Length: %d)", strlen(global_config.wifi_password));

    lv_obj_t * button_row = lv_create_empty_panel(root);
    lv_layout_flex_row(button_row, LV_FLEX_ALIGN_SPACE_BETWEEN);
    lv_obj_set_size(button_row, lv_pct(100), CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t * no_btn = lv_btn_create(button_row);
    lv_obj_add_event_cb(no_btn, btn_no_reuse_password, LV_EVENT_CLICKED, (void*)ssid);
    lv_obj_set_height(no_btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_set_width(no_btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX);
    lv_obj_set_style_pad_all(no_btn, CYD_SCREEN_GAP_PX, 0);

    label = lv_label_create(no_btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE);
    lv_obj_center(label);

    lv_obj_t * yes_btn = lv_btn_create(button_row);
    lv_obj_add_event_cb(yes_btn, btn_reuse_password, LV_EVENT_CLICKED, (void*)ssid);
    lv_obj_set_height(yes_btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_set_width(yes_btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX);
    lv_obj_set_style_pad_all(yes_btn, CYD_SCREEN_GAP_PX, 0);

    label = lv_label_create(yes_btn);
    lv_label_set_text(label, LV_SYMBOL_OK);
    lv_obj_center(label);
}

void wifi_pass_entry(const char* ssid)
{
    current_ssid_ptr = ssid;

    if (strcmp(ssid, global_config.wifi_SSID) == 0){
        ask_reuse_password(ssid);
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

    lv_obj_t * label = lv_label_create(top_root);
    lv_label_set_text(label, "Enter WiFi Password");
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);

    lv_obj_t * passEntry = lv_textarea_create(top_root);
    lv_textarea_set_one_line(passEntry, true);
    lv_textarea_set_text(passEntry, "");
    lv_obj_set_width(passEntry, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_add_event_cb(passEntry, ta_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_flex_grow(passEntry, 1);

    lv_obj_t * keyboard = lv_keyboard_create(root);
    lv_keyboard_set_textarea(keyboard, passEntry);
}

static void wifi_btn_event_handler(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        delay(100);
        char* ssid = (char*)e->user_data;
        Serial.println(ssid);
        wifi_pass_entry(ssid);
    }
}

void wifi_init_inner(){
    WiFi.disconnect();
    lv_obj_clean(lv_scr_act());

    if (global_config.wifi_configured){
        WiFi.begin(global_config.wifi_SSID, global_config.wifi_password);
        
        Serial.printf("Connecting to %s with a password length of %d\n", global_config.wifi_SSID, strlen(global_config.wifi_password));

        lv_obj_t * label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "Connecting to WiFi");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t * resetBtn = lv_btn_create(lv_scr_act());
        lv_obj_add_event_cb(resetBtn, reset_btn_event_handler, LV_EVENT_CLICKED, NULL);
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

    lv_obj_t * root = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(root, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_layout_flex_column(root);
    lv_obj_set_style_pad_all(root, CYD_SCREEN_GAP_PX, 0);

    lv_obj_t * top_row = lv_create_empty_panel(root);
    lv_obj_set_size(top_row, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, LV_SIZE_CONTENT);
    lv_layout_flex_row(top_row, LV_FLEX_ALIGN_SPACE_BETWEEN);

    label = lv_label_create(top_row);
    lv_label_set_text(label, "Select a network");

    lv_obj_t * refreshBtn = lv_btn_create(top_row);
    lv_obj_add_event_cb(refreshBtn, reset_btn_event_handler, LV_EVENT_ALL, NULL);
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
        lv_obj_add_event_cb(btn, wifi_btn_event_handler, LV_EVENT_ALL, (void*)ssid_copy);
    }
}

const char* errs[] = {
    "Idle",
    "No SSID Available",
    "Scan Completed",
    "Connected",
    "Connection Failed",
    "Connection Lost",
    "Disconnected"
};

const int print_freq = 1000;
int print_timer = 0;

void wifi_init(){
    WiFi.mode(WIFI_STA);
    wifi_init_inner();

    while (!global_config.wifi_configured || WiFi.status() != WL_CONNECTED){
        if (millis() - print_timer > print_freq){
            print_timer = millis();
            Serial.printf("WiFi Status: %s\n", errs[WiFi.status()]);
        }
        
        lv_timer_handler();
        lv_task_handler();
    }
}

void wifi_ok(){
    if (WiFi.status() != WL_CONNECTED){
        ESP.restart();
    }
}