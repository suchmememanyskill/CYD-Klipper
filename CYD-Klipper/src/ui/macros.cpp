#include "macros.h"
#include "ui_utils.h"
#include <Esp.h>
#include "../core/data_setup.h"

PRINTER_CONFIG * curernt_config = NULL;

static void btn_press(lv_event_t * e){
    lv_obj_t * btn = lv_event_get_target(e);
    const char* macro = (const char*)lv_event_get_user_data(e);
    LOG_F(("Macro: %s\n", macro))
    send_gcode(false, macro);
}

void macros_add_macros_to_panel(lv_obj_t * root_panel, MACROSQUERY query)
{
    for (int i = 0; i < query.count; i++){
        const char* macro = query.macros[i];
        lv_create_custom_menu_button(macro, root_panel, btn_press, "Run", (void*)macro);
    }
}

static void power_device_toggle(lv_event_t * e)
{
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    const char* power_device_name = (const char*)lv_event_get_user_data(e);
    LOG_F(("Power Device: %s, State: %d -> %d\n", power_device_name, !checked, checked))

    if (curernt_config != NULL)
        set_power_state(power_device_name, checked, curernt_config);
}

void macros_add_power_devices_to_panel(lv_obj_t * root_panel, POWERQUERY query)
{
    for (int i = 0; i < query.count; i++){
        const char* power_device_name = query.power_devices[i];
        const bool power_device_state = query.power_states[i];
        lv_create_custom_menu_switch(power_device_name, root_panel, power_device_toggle, power_device_state, (void*)power_device_name);
    }
}

void macros_set_current_config(PRINTER_CONFIG * config)
{
    curernt_config = config;
}

void macros_draw_power_fullscreen(PRINTER_CONFIG * config)
{
    macros_set_current_config(config);

    lv_obj_t * parent = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_style_bg_opa(parent, LV_OPA_100, 0); 
    lv_obj_align(parent, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_size(parent, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_layout_flex_column(parent);

    lv_obj_set_size(lv_create_empty_panel(parent), 0, 0);

    auto width = CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;

    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, destroy_event_user_data, LV_EVENT_CLICKED, parent);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " Close");
    lv_obj_center(label);

    POWERQUERY power = power_devices_query(config);
    macros_add_power_devices_to_panel(parent, power);
}

void macros_draw_power_fullscreen()
{
    macros_draw_power_fullscreen(get_current_printer_config());
}