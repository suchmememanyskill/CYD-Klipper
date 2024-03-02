#include "lvgl.h"
#include "panels/panel.h"
#include "../core/data_setup.h"
#include "nav_buttons.h"
#include "ui_utils.h"
#include <stdio.h>

static lv_style_t navButtonStyle;

static char tempBuffer[10];
static char zPosBuffer[10];
static char timeBuffer[10];

static lv_style_t navButtonTextStyle;

static void UpdatePrinterDataZPos(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);

    sprintf(zPosBuffer, "Z%.2f", printer.position[2]);
    lv_label_set_text(label, zPosBuffer);
}

static void UpdatePrinterDataTemp(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);

    sprintf(tempBuffer, "%.0f/%.0f", printer.extruderTemp, printer.bedTemp);
    lv_label_set_text(label, tempBuffer);
}

static void UpdatePrinterDataTime(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);

    if (printer.state == PRINTER_STATE_IDLE){
        lv_label_set_text(label, "Idle");
        return;
    }

    if (printer.state == PRINTER_STATE_PAUSED){
        lv_label_set_text(label, "Paused");
        return;
    }

    unsigned long time = printer.remainingTime;
    unsigned long hours = time / 3600;
    unsigned long minutes = (time % 3600) / 60;
    unsigned long seconds = (time % 3600) % 60;

    if (hours >= 10){
        sprintf(timeBuffer, "%luh", hours);
    } else if (hours >= 1){
        sprintf(timeBuffer, "%luh%02lum", hours, minutes);
    } else {
        sprintf(timeBuffer, "%lum", minutes);
    }

    lv_label_set_text(label, timeBuffer);
}

static void BtnClickFiles(lv_event_t * e){
    NavButtonsSetup(0);
}

static void BtnClickMove(lv_event_t * e){
    NavButtonsSetup(1);
}

static void BtnClickExtrude(lv_event_t * e){
    NavButtonsSetup(2);
}

static void BtnClickSettings(lv_event_t * e){
    NavButtonsSetup(3);
}

static void BtnClickMacros(lv_event_t * e){
    NavButtonsSetup(4);
}

void CreateButton(const char* icon, const char* name, lv_event_cb_t buttonClick, lv_event_cb_t labelUpdate, lv_obj_t * root){
    lv_obj_t* btn = lv_btn_create(root);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_width(btn, CYD_SCREEN_SIDEBAR_SIZE_PX);
    lv_obj_add_style(btn, &navButtonStyle, 0);
    if (buttonClick != NULL)
        lv_obj_add_event_cb(btn, buttonClick, LV_EVENT_CLICKED, NULL);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, icon);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -1 * CYD_SCREEN_GAP_PX);

    label = lv_label_create(btn);
    lv_label_set_text(label, name);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, CYD_SCREEN_GAP_PX);
    lv_obj_add_event_cb(label, labelUpdate, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);
    lv_obj_add_style(label, &navButtonTextStyle, 0);
}

void NavButtonsSetup(unsigned char activePanel){
    lv_obj_clean(lv_scr_act());
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * rootPanel = CreateEmptyPanel(lv_scr_act());
    lv_obj_set_size(rootPanel, CYD_SCREEN_SIDEBAR_SIZE_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_align(rootPanel, LV_ALIGN_TOP_LEFT, 0, 0);
    LayoutFlexColumn(rootPanel, LV_FLEX_ALIGN_START, 0, 0);

    // Files/Print
    CreateButton(LV_SYMBOL_COPY, "Idle", BtnClickFiles, UpdatePrinterDataTime, rootPanel);

    // Move
    CreateButton(printer.state == PRINTER_STATE_PRINTING ? LV_SYMBOL_EDIT : LV_SYMBOL_CHARGE, "Z?", BtnClickMove, UpdatePrinterDataZPos, rootPanel);

    // Extrude/Temp
    CreateButton(LV_SYMBOL_WARNING, "?/?", BtnClickExtrude, UpdatePrinterDataTemp, rootPanel);

    // Macros
    CreateButton(LV_SYMBOL_GPS, "Macro", BtnClickMacros, NULL, rootPanel);

    lv_obj_t * panel = CreateEmptyPanel(lv_scr_act());
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_align(panel, LV_ALIGN_TOP_RIGHT, 0, 0);

    switch (activePanel){
        case 0:
            PrintPanelInit(panel);
            break;
        case 1:
            MovePanelInit(panel);
            break;
        case 2:
            TempPanelInit(panel);
            break;
        case 3:
            SettingsPanelInit(panel);
            break;
        case 4:
            MacrosPanelInit(panel);
            break;
        case 5:
            StatsPanelInit(panel);
            break;
    }

    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

void NavStyleSetup(){
    lv_style_init(&navButtonStyle);
    lv_style_set_radius(&navButtonStyle, 0);

    lv_style_init(&navButtonTextStyle);
    lv_style_set_text_font(&navButtonTextStyle, &CYD_SCREEN_FONT_SMALL);
}
