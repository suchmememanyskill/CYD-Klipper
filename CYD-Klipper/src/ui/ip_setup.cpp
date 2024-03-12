#include "ip_setup.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include <HTTPClient.h>
#include "core/data_setup.h"
#include "ui_utils.h"
#include "../core/macros_query.h"
#include "panels/panel.h"
#include "../core/http_client.h"
#include "switch_printer.h"
#include "macros.h"

bool connect_ok = false;
int prev_power_device_count = 0;
lv_obj_t * hostEntry;
lv_obj_t * portEntry;
lv_obj_t * label = NULL;

/* Create a custom keyboard to allow hostnames or ip addresses (a-z, 0 - 9, and -) */
static const char * kb_map[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_OK, "\n",
    LV_SYMBOL_LEFT, "z", "x", "c", "v", "b", "n", "m", ".", "-", LV_SYMBOL_RIGHT, NULL
};

static const lv_btnmatrix_ctrl_t kb_ctrl[] = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 6,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 5,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 6, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 6
};

void ip_init_inner();

enum connection_status_t {
    CONNECT_FAIL = 0,
    CONNECT_OK = 1,
    CONNECT_AUTH_REQUIRED = 2,
};

connection_status_t verify_ip(){
    SETUP_HTTP_CLIENT_FULL("/printer/info", true, 1000);

    int httpCode;
    try {
        httpCode = client.GET();

        if (httpCode == 401)
            return CONNECT_AUTH_REQUIRED;

        return httpCode == 200 ? CONNECT_OK : CONNECT_FAIL;
    }
    catch (...) {
        Serial.println("Failed to connect");
        return CONNECT_FAIL;
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
        strcpy(get_current_printer_config()->klipper_host, lv_textarea_get_text(hostEntry));
        get_current_printer_config()->klipper_port = atoi(lv_textarea_get_text(portEntry));

        connection_status_t status = verify_ip();
        if (status == CONNECT_OK)
        {
            get_current_printer_config()->ip_configured = true;
            write_global_config();
            connect_ok = true;
        }
        else if (status == CONNECT_AUTH_REQUIRED)
        {
            label = NULL;
            get_current_printer_config()->ip_configured = true;
            write_global_config();
        }
        else
        {
            lv_label_set_text(label, "Failed to connect");
        }
    }
    else
    {
        return;
    }

    if (lv_obj_has_flag(ta, LV_OBJ_FLAG_USER_1))
    {
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
    }
    else
    {
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    }
}

static void reset_btn_event_handler(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        get_current_printer_config()->ip_configured = false;
        ip_init_inner();
    }
}

static void power_devices_button(lv_event_t * e) {
    macros_draw_power_fullscreen();
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

    if (prev_power_device_count >= 1){
        lv_obj_t * power_devices_btn = lv_btn_create(button_row);
        lv_obj_add_event_cb(power_devices_btn, power_devices_button, LV_EVENT_CLICKED, NULL);
        lv_obj_set_height(power_devices_btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        btn_label = lv_label_create(power_devices_btn);
        lv_label_set_text(btn_label, "Power Devices");
        lv_obj_center(btn_label);
    }

    draw_switch_printer_button();    
}

static bool auth_entry_done = false;

static void keyboard_event_auth_entry(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_READY) 
    {
        const char * txt = lv_textarea_get_text(ta);
        int len = strlen(txt);
        if (len > 0)
        {
            get_current_printer_config()->auth_configured = true;
            strcpy(get_current_printer_config()->klipper_auth, txt);
            write_global_config();
            auth_entry_done = true;
        }
    }
    else if (code == LV_EVENT_CANCEL)
    {
        auth_entry_done = true;
    }
}

void handle_auth_entry(){
    auth_entry_done = false;
    get_current_printer_config()->klipper_auth[32] = 0;
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
    lv_label_set_text(label, "Enter API Key");
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);

    lv_obj_t * keyboard = lv_keyboard_create(root);
    lv_obj_t * passEntry = lv_textarea_create(top_root);
    lv_textarea_set_max_length(passEntry, 32);
    lv_textarea_set_one_line(passEntry, true);

    if (get_current_printer_config()->auth_configured)
        lv_textarea_set_text(passEntry, get_current_printer_config()->klipper_auth);
    else
        lv_textarea_set_text(passEntry, "");

    lv_obj_set_width(passEntry, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_add_event_cb(passEntry, keyboard_event_auth_entry, LV_EVENT_ALL, keyboard);
    lv_obj_set_flex_grow(passEntry, 1);
    

    lv_keyboard_set_textarea(keyboard, passEntry);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, kb_map, kb_ctrl);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);

    while (!auth_entry_done) {
        lv_timer_handler();
        lv_task_handler();
    }

    redraw_connect_screen();
}

void ip_init_inner(){
    if (get_current_printer_config()->ip_configured) {
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
    lv_label_set_text(label, "Enter Klipper IP/Hostname and Port");
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);

    lv_obj_t * textbow_row = lv_create_empty_panel(top_root);
    lv_obj_set_width(textbow_row, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_set_flex_grow(textbow_row, 1);
    lv_layout_flex_row(textbow_row);

    hostEntry = lv_textarea_create(textbow_row);
    lv_textarea_set_one_line(hostEntry, true);
    lv_obj_add_flag(hostEntry, LV_OBJ_FLAG_USER_1);
    lv_textarea_set_max_length(hostEntry, 63);
    lv_textarea_set_text(hostEntry, "");
    lv_obj_set_flex_grow(hostEntry, 3);

    portEntry = lv_textarea_create(textbow_row);
    lv_textarea_set_one_line(portEntry, true);
    lv_textarea_set_max_length(portEntry, 5);
    lv_textarea_set_text(portEntry, "80");
    lv_obj_set_flex_grow(portEntry, 1);

    lv_obj_t * keyboard = lv_keyboard_create(root);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, kb_map, kb_ctrl);
    lv_obj_add_event_cb(hostEntry, ta_event_cb, LV_EVENT_ALL, keyboard);
    lv_obj_add_event_cb(portEntry, ta_event_cb, LV_EVENT_ALL, keyboard);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);
    lv_keyboard_set_textarea(keyboard, hostEntry);
}

long last_data_update_ip = -10000;
const long data_update_interval_ip = 10000;
int retry_count = 0;

void ip_init(){
    connect_ok = false;
    retry_count = 0;
    prev_power_device_count = 0;

    ip_init_inner();

    while (!connect_ok)
    {
        lv_timer_handler();
        lv_task_handler();

        if (!connect_ok && get_current_printer_config()->ip_configured && (millis() - last_data_update_ip) > data_update_interval_ip){
            connection_status_t status = verify_ip();

            connect_ok = status == CONNECT_OK;
            last_data_update_ip = millis();
            retry_count++;
            if (label != NULL){
                String retry_count_text = "Connecting to Klipper (Try " + String(retry_count + 1) + ")";
                lv_label_set_text(label, retry_count_text.c_str());
            }

            if (status == CONNECT_AUTH_REQUIRED)
                handle_auth_entry();

            unsigned int power_device_count = power_devices_count();
            if (power_device_count != prev_power_device_count) {
                prev_power_device_count = power_device_count;
                redraw_connect_screen();
            }
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