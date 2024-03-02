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
char extruderTempBuff[20];
char bedTempBuff[20];
char positionBuff[20];

static void BtnClickRestart(lv_event_t * e){
    SendGcode(false, "RESTART");
}

static void BtnClickFirmwareRestart(lv_event_t * e){
    SendGcode(false, "FIRMWARE_RESTART");
}

void ErrorUiMacrosOpen(lv_event_t * e){
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

void ErrorUi(){
    lv_obj_clean(lv_scr_act());

    lv_obj_t * panel = CreateEmptyPanel(lv_scr_act());
    LayoutFlexColumn(panel);
    lv_obj_set_size(panel, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t * label;
    label = lv_label_create(panel);
    lv_label_set_text(label, LV_SYMBOL_WARNING " Printer is not ready");

    label = lv_label_create(panel);
    lv_label_set_text(label, printer.stateMessage);
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

    lv_obj_t * buttonRow = CreateEmptyPanel(panel);
    lv_obj_set_size(buttonRow, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    LayoutFlexRow(buttonRow);

    lv_obj_t * btn = lv_btn_create(buttonRow);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, BtnClickRestart, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Restart");
    lv_obj_center(label);

    btn = lv_btn_create(buttonRow);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, BtnClickFirmwareRestart, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, "FW Restart");
    lv_obj_center(label);

    if (PowerDevicesQuery().count >= 1){
        btn = lv_btn_create(buttonRow);
        lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_obj_add_event_cb(btn, ErrorUiMacrosOpen, LV_EVENT_CLICKED, NULL);
        lv_obj_set_flex_grow(btn, 1);

        label = lv_label_create(btn);
        lv_label_set_text(label, "Devices");
        lv_obj_center(label);
    }
}

void CheckIfScreenNeedsToBeDisabled(){
    if (globalConfig.onDuringPrint && printer.state == PRINTER_STATE_PRINTING){
        ScreenTimerWake();
        ScreenTimerStop();
    }
    else {
        ScreenTimerStart();
    }
}

static void OnStateChange(void * s, lv_msg_t * m){
    CheckIfScreenNeedsToBeDisabled();

    if (printer.state == PRINTER_STATE_ERROR){
        ErrorUi();
    }
    else {
        NavButtonsSetup(0);
    }
}

void MainUiSetup(){
    lv_msg_subscribe(DATA_PRINTER_STATE, OnStateChange, NULL);
    OnStateChange(NULL, NULL);
}
