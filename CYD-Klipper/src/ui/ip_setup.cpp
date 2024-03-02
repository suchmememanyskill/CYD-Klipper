#include "ip_setup.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include <HTTPClient.h>
#include "core/data_setup.h"
#include "ui_utils.h"
#include "../core/macros_query.h"
#include "panels/panel.h"

bool ConnectOK = false;
lv_obj_t * HostEntry;
lv_obj_t * PortEntry;
lv_obj_t * Label = NULL;

static const char * KB_MAP[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_OK, "\n",
    LV_SYMBOL_LEFT, "z", "x", "c", "v", "b", "n", "m", ".", "-", LV_SYMBOL_RIGHT, NULL
};

static const lv_btnmatrix_ctrl_t KB_CTRL[] = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 6,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 5,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 6, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 6
};

void IpInitInner();

enum ConnectionStatusT {
    CONNECT_FAIL = 0,
    CONNECT_OK = 1,
    CONNECT_AUTH_REQUIRED = 2,
};

ConnectionStatusT VerifyIp(){
    HTTPClient client;
    String url = "http://" + String(globalConfig.klipperHost) + ":" + String(globalConfig.klipperPort) + "/printer/info";
    int httpCode;
    try {
        client.setTimeout(500);
        client.setConnectTimeout(1000);
        client.begin(url.c_str());

        if (globalConfig.authConfigured)
            client.addHeader("X-Api-Key", globalConfig.klipperAuth);

        httpCode = client.GET();
        Serial.printf("%d %s\n", httpCode, url.c_str());

        if (httpCode == 401)
            return CONNECT_AUTH_REQUIRED;

        return httpCode == 200 ? CONNECT_OK : CONNECT_FAIL;
    }
    catch (...) {
        Serial.println("Failed to connect");
        return CONNECT_FAIL;
    }
}

static void TaEventCb(lv_event_t * e) {
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
        strcpy(globalConfig.klipperHost, lv_textarea_get_text(HostEntry));
        globalConfig.klipperPort = atoi(lv_textarea_get_text(PortEntry));

        ConnectionStatusT status = VerifyIp();
        if (status == CONNECT_OK)
        {
            globalConfig.ipConfigured = true;
            WriteGlobalConfig();
            ConnectOK = true;
        }
        else if (status == CONNECT_AUTH_REQUIRED)
        {
            Label = NULL;
            globalConfig.ipConfigured = true;
            WriteGlobalConfig();
        }
        else
        {
            lv_label_set_text(Label, "Failed to connect");
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

static void ResetBtnEventHandler(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        globalConfig.ipConfigured = false;
        IpInitInner();
    }
}

static void PowerDevicesButton(lv_event_t * e) {
    lv_obj_t * panel = CreateEmptyPanel(lv_scr_act());
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    LayoutFlexColumn(panel);
    lv_obj_set_size(panel, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, 0, CYD_SCREEN_GAP_PX);

    lv_obj_t * button = lv_btn_create(panel);
    lv_obj_set_size(button, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(button, DestroyEventUserData, LV_EVENT_CLICKED, panel);

    lv_obj_t * label = lv_label_create(button);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " Close");
    lv_obj_center(label);

    MacrosPanelAddPowerDevicesToPanel(panel, PowerDevicesQuery());
}

void RedrawConnectScreen(){
    lv_obj_clean(lv_scr_act());

    Label = lv_label_create(lv_scr_act());
    lv_label_set_text(Label, "Connecting to Klipper");
    lv_obj_align(Label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * button_row = CreateEmptyPanel(lv_scr_act());
    lv_obj_set_size(button_row, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    LayoutFlexRow(button_row, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(button_row, LV_ALIGN_CENTER, 0, CYD_SCREEN_GAP_PX + CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t * reset_btn = lv_btn_create(button_row);
    lv_obj_add_event_cb(reset_btn, ResetBtnEventHandler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_height(reset_btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t * btn_label = lv_label_create(reset_btn);
    lv_label_set_text(btn_label, "Reset");
    lv_obj_center(btn_label);

    if (PowerDevicesQuery().count >= 1){
        lv_obj_t * power_devices_btn = lv_btn_create(button_row);
        lv_obj_add_event_cb(power_devices_btn, PowerDevicesButton, LV_EVENT_CLICKED, NULL);
        lv_obj_set_height(power_devices_btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        btn_label = lv_label_create(power_devices_btn);
        lv_label_set_text(btn_label, "Power Devices");
        lv_obj_center(btn_label);
    }
}

static bool AuthEntryDone = false;

static void KeyboardEventAuthEntry(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_READY)
    {
        const char * txt = lv_textarea_get_text(ta);
        int len = strlen(txt);
        if (len > 0)
        {
            globalConfig.authConfigured = true;
            strcpy(globalConfig.klipperAuth, txt);
            WriteGlobalConfig();
            AuthEntryDone = true;
        }
    }
    else if (code == LV_EVENT_CANCEL)
    {
        AuthEntryDone = true;
    }
}

void HandleAuthEntry(){
    AuthEntryDone = false;
    globalConfig.klipperAuth[32] = 0;
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
    lv_label_set_text(label, "Enter API Key");
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);

    lv_obj_t * keyboard = lv_keyboard_create(root);
    lv_obj_t * passEntry = lv_textarea_create(top_root);
    lv_textarea_set_max_length(passEntry, 32);
    lv_textarea_set_one_line(passEntry, true);

    if (globalConfig.authConfigured)
        lv_textarea_set_text(passEntry, globalConfig.klipperAuth);
    else
        lv_textarea_set_text(passEntry, "");

    lv_obj_set_width(passEntry, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_add_event_cb(passEntry, KeyboardEventAuthEntry, LV_EVENT_ALL, keyboard);
    lv_obj_set_flex_grow(passEntry, 1);

    lv_keyboard_set_textarea(keyboard, passEntry);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, KB_MAP, KB_CTRL);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);

    while (!AuthEntryDone) {
        lv_timer_handler();
        lv_task_handler();
    }

    RedrawConnectScreen();
}

void IpInitInner(){
    if (globalConfig.ipConfigured) {
        RedrawConnectScreen();
        return;
    }

    lv_obj_clean(lv_scr_act());

    lv_obj_t * root = CreateEmptyPanel(lv_scr_act());
    lv_obj_set_size(root, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    LayoutFlexColumn(root);

    lv_obj_t * top_root = CreateEmptyPanel(root);
    lv_obj_set_width(top_root, CYD_SCREEN_WIDTH_PX);
    LayoutFlexColumn(top_root);
    lv_obj_set_flex_grow(top_root, 1);
    lv_obj_set_style_pad_all(top_root, CYD_SCREEN_GAP_PX, 0);

    Label = lv_label_create(top_root);
    lv_label_set_text(Label, "Enter Klipper IP/Hostname and Port");
    lv_obj_set_width(Label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);

    lv_obj_t * textbow_row = CreateEmptyPanel(top_root);
    lv_obj_set_width(textbow_row, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_set_flex_grow(textbow_row, 1);
    LayoutFlexRow(textbow_row);

    HostEntry = lv_textarea_create(textbow_row);
    lv_textarea_set_one_line(HostEntry, true);
    lv_obj_add_flag(HostEntry, LV_OBJ_FLAG_USER_1);
    lv_textarea_set_max_length(HostEntry, 63);
    lv_textarea_set_text(HostEntry, "");
    lv_obj_set_flex_grow(HostEntry, 3);

    PortEntry = lv_textarea_create(textbow_row);
    lv_textarea_set_one_line(PortEntry, true);
    lv_textarea_set_max_length(PortEntry, 5);
    lv_textarea_set_text(PortEntry, "80");
    lv_obj_set_flex_grow(PortEntry, 1);

    lv_obj_t * keyboard = lv_keyboard_create(root);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, KB_MAP, KB_CTRL);
    lv_obj_add_event_cb(HostEntry, TaEventCb, LV_EVENT_ALL, keyboard);
    lv_obj_add_event_cb(PortEntry, TaEventCb, LV_EVENT_ALL, keyboard);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);
    lv_keyboard_set_textarea(keyboard, HostEntry);
}

long LastDataUpdateIp = -10000;
const long DataUpdateIntervalIp = 10000;
int RetryCount = 0;

void IpInit(){
    ConnectOK = false;
    RetryCount = 0;
    int PrevPowerDeviceCount = 0;

    IpInitInner();

    while (!ConnectOK)
    {
        lv_timer_handler();
        lv_task_handler();

        if (!ConnectOK && globalConfig.ipConfigured && (millis() - LastDataUpdateIp) > DataUpdateIntervalIp){
            ConnectionStatusT status = VerifyIp();

            ConnectOK = status == CONNECT_OK;
            LastDataUpdateIp = millis();
            RetryCount++;
            if (Label != NULL){
                String RetryCountText = "Connecting to Klipper (Try " + String(RetryCount + 1) + ")";
                lv_label_set_text(Label, RetryCountText.c_str());
            }

            if (status != CONNECT_AUTH_REQUIRED)
                PowerDevicesQueryInternal();
            else
                HandleAuthEntry();

            if (PowerDevicesQuery().count != PrevPowerDeviceCount) {
                PrevPowerDeviceCount = PowerDevicesQuery().count;
                RedrawConnectScreen();
            }
        }
    }
}

void IpOk(){
    if (klipperRequestConsecutiveFailCount > 5){
        FreezeRequestThread();
        PowerDevicesClear();
        IpInit();
        UnfreezeRequestThread();
        klipperRequestConsecutiveFailCount = 0;
        lv_msg_send(DATA_PRINTER_STATE, &printer);
    }
}
