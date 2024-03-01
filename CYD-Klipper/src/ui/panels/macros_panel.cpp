#include "lvgl.h"
#include "panel.h"
#include "../nav_buttons.h"
#include "../../core/data_setup.h"
#include "../../core/macros_query.h"
#include "../../conf/global_config.h"
#include "../ui_utils.h"
#include <HardwareSerial.h>

const static lv_point_t line_points[] = { {0, 0}, {(short int)((CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2) * 0.85f), 0} };

static void btn_press(lv_event_t * e){
    lv_obj_t * btn = lv_event_get_target(e);
    const char* macro = (const char*)lv_event_get_user_data(e);
    Serial.printf("Macro: %s\n", macro);
    send_gcode(false, macro);
}

static void btn_goto_settings(lv_event_t * e){
    nav_buttons_setup(3);
}

void macros_panel_add_macros_to_panel(lv_obj_t * root_panel, MACROSQUERY query){
    for (int i = 0; i < query.count; i++){
        const char* macro = query.macros[i];
        
        lv_obj_t * panel = lv_create_empty_panel(root_panel);
        lv_layout_flex_row(panel, LV_FLEX_ALIGN_END);
        lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        lv_obj_t * label = lv_label_create(panel);
        lv_label_set_text(label, macro);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);

        lv_obj_t * btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, btn_press, LV_EVENT_CLICKED, (void*)macro);
        lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        label = lv_label_create(btn);
        lv_label_set_text(label, "Run");
        lv_obj_center(label);

        lv_obj_t * line = lv_line_create(root_panel);
        lv_line_set_points(line, line_points, 2);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_color(line, lv_color_hex(0xAAAAAA), 0);
    }
}

static void power_device_toggle(lv_event_t * e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    const char* power_device_name = (const char*)lv_event_get_user_data(e);
    Serial.printf("Power Device: %s, State: %d -> %d\n", power_device_name, !checked, checked);

    set_power_state(power_device_name, checked);
}

void macros_panel_add_power_devices_to_panel(lv_obj_t * root_panel, POWERQUERY query){
    for (int i = 0; i < query.count; i++){
        const char* power_device_name = query.power_devices[i];
        const bool power_device_state = query.power_states[i];
        
        lv_obj_t * panel = lv_create_empty_panel(root_panel);
        lv_layout_flex_row(panel, LV_FLEX_ALIGN_END);
        lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        lv_obj_t * label = lv_label_create(panel);
        lv_label_set_text(label, power_device_name);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);

        lv_obj_t * toggle = lv_switch_create(panel);
        lv_obj_add_event_cb(toggle, power_device_toggle, LV_EVENT_VALUE_CHANGED, (void*)power_device_name);
        lv_obj_set_size(toggle, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        if (power_device_state)
            lv_obj_add_state(toggle, LV_STATE_CHECKED);

        lv_obj_t * line = lv_line_create(root_panel);
        lv_line_set_points(line, line_points, 2);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_color(line, lv_color_hex(0xAAAAAA), 0);
    }
}

void macros_panel_init(lv_obj_t* panel) {
    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_add_event_cb(btn, btn_goto_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, CYD_SCREEN_GAP_PX);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS " Screen Settings");
    lv_obj_center(label);

    MACROSQUERY query = macros_query();
    POWERQUERY power = power_devices_query();
    if (query.count == 0 && power.count == 0){
        label = lv_label_create(panel);
        lv_label_set_text(label, "No macros found.\nMacros with the description\n\"CYD_SCREEN_MACRO\"\nwill show up here.");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    lv_obj_t * root_panel = lv_create_empty_panel(panel);
    lv_obj_set_scrollbar_mode(root_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(root_panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX - CYD_SCREEN_MIN_BUTTON_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2); 
    lv_obj_align(root_panel, LV_ALIGN_TOP_MID, 0, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX + CYD_SCREEN_GAP_PX * 2);
    lv_layout_flex_column(root_panel);

    macros_panel_add_power_devices_to_panel(root_panel, power);
    macros_panel_add_macros_to_panel(root_panel, query);
}