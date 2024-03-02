#include "lvgl.h"
#include "panel.h"
#include "../nav_buttons.h"
#include "../../core/data_setup.h"
#include "../../core/macros_query.h"
#include "../../conf/global_config.h"
#include "../ui_utils.h"
#include <HardwareSerial.h>

const static lv_point_t LINE_POINTS[] = { {0, 0}, {(short int)((CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2) * 0.85f), 0} };

static void BtnPress(lv_event_t * e){
    lv_obj_t * btn = lv_event_get_target(e);
    const char* macro = (const char*)lv_event_get_user_data(e);
    Serial.printf("Macro: %s\n", macro);
    SendGcode(false, macro);
}

static void BtnGotoSettings(lv_event_t * e){
    NavButtonsSetup(3);
}

void MacrosPanelAddMacrosToPanel(lv_obj_t * root_panel, MacrosQueryT query){
    for (int i = 0; i < query.count; i++){
        const char* macro = query.macros[i];

        lv_obj_t * panel = CreateEmptyPanel(root_panel);
        LayoutFlexRow(panel, LV_FLEX_ALIGN_END);
        lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        lv_obj_t * label = lv_label_create(panel);
        lv_label_set_text(label, macro);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);

        lv_obj_t * btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, BtnPress, LV_EVENT_CLICKED, (void*)macro);
        lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        label = lv_label_create(btn);
        lv_label_set_text(label, "Run");
        lv_obj_center(label);

        lv_obj_t * line = lv_line_create(root_panel);
        lv_line_set_points(line, LINE_POINTS, 2);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_color(line, lv_color_hex(0xAAAAAA), 0);
    }
}

static void PowerDeviceToggle(lv_event_t * e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    const char* power_device_name = (const char*)lv_event_get_user_data(e);
    Serial.printf("Power Device: %s, State: %d -> %d\n", power_device_name, !checked, checked);

    SetPowerState(power_device_name, checked);
}

void MacrosPanelAddPowerDevicesToPanel(lv_obj_t * root_panel, PowerQueryT query){
    for (int i = 0; i < query.count; i++){
        const char* power_device_name = query.powerDevices[i];
        const bool power_device_state = query.powerStates[i];

        lv_obj_t * panel = CreateEmptyPanel(root_panel);
        LayoutFlexRow(panel, LV_FLEX_ALIGN_END);
        lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        lv_obj_t * label = lv_label_create(panel);
        lv_label_set_text(label, power_device_name);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);

        lv_obj_t * toggle = lv_switch_create(panel);
        lv_obj_add_event_cb(toggle, PowerDeviceToggle, LV_EVENT_VALUE_CHANGED, (void*)power_device_name);
        lv_obj_set_size(toggle, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        if (power_device_state)
            lv_obj_add_state(toggle, LV_STATE_CHECKED);

        lv_obj_t * line = lv_line_create(root_panel);
        lv_line_set_points(line, LINE_POINTS, 2);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_color(line, lv_color_hex(0xAAAAAA), 0);
    }
}


void MacrosPanelInit(lv_obj_t* panel) {
    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_add_event_cb(btn, BtnGotoSettings, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, CYD_SCREEN_GAP_PX);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS " Screen Settings");
    lv_obj_center(label);

    MacrosQueryT query = MacrosQuery();
    PowerQueryT power = PowerDevicesQuery();
    if (query.count == 0 && power.count == 0){
        label = lv_label_create(panel);
        lv_label_set_text(label, "No macros found.\nMacros with the description\n\"CYD_SCREEN_MACRO\"\nwill show up here.");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    lv_obj_t * root_panel = CreateEmptyPanel(panel);
    lv_obj_set_scrollbar_mode(root_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(root_panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_MIN_BUTTON_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_align(root_panel, LV_ALIGN_TOP_MID, 0, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX + CYD_SCREEN_GAP_PX * 2);
    LayoutFlexColumn(root_panel);

    MacrosPanelAddPowerDevicesToPanel(root_panel, power);
    MacrosPanelAddMacrosToPanel(root_panel, query);
}
