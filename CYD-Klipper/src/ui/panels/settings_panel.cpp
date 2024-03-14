#include "lvgl.h"
#include "panel.h"
#include "../../core/screen_driver.h"
#include "../../conf/global_config.h"
#include "../main_ui.h"
#include "../ui_utils.h"
#include <Esp.h>
#include "../../core/lv_setup.h"
#include "../ota_setup.h"
#include "../nav_buttons.h"

#ifndef REPO_VERSION
    #define REPO_VERSION "Unknown"
#endif // REPO_VERSION

static void invert_color_switch(lv_event_t * e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    get_current_printer_config()->invert_colors = checked;
    write_global_config();
    set_invert_display();
}

static void reset_calibration_click(lv_event_t * e){
    global_config.screen_calibrated = false;
    write_global_config();
    ESP.restart();
}

static void reset_wifi_click(lv_event_t * e){
    global_config.wifi_configured = false;
    write_global_config();
    ESP.restart();
}

static void reset_ip_click(lv_event_t * e){
    get_current_printer_config()->ip_configured = false;
    get_current_printer_config()->auth_configured = false;
    write_global_config();
    ESP.restart();
}

static void light_mode_switch(lv_event_t * e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    get_current_printer_config()->light_mode = checked;
    write_global_config();
    set_color_scheme();
}

static void theme_dropdown(lv_event_t * e){
    lv_obj_t * dropdown = lv_event_get_target(e);
    auto selected = lv_dropdown_get_selected(dropdown);
    get_current_printer_config()->color_scheme = selected;
    set_color_scheme();
    write_global_config();
}

const char* brightness_options = "100%\n75%\n50%\n25%";
const unsigned char  brightness_options_values[] = { 255, 192, 128, 64 };

static void brightness_dropdown(lv_event_t * e){
    lv_obj_t * dropdown = lv_event_get_target(e);
    auto selected = lv_dropdown_get_selected(dropdown);
    global_config.brightness = brightness_options_values[selected];
    set_screen_brightness();
    write_global_config();
}

const char* wake_timeout_options = "1m\n2m\n5m\n10m\n15m\n30m\n1h\n2h\n4h";
const char  wake_timeout_options_values[] = { 1, 2, 5, 10, 15, 30, 60, 120, 240 };

static void wake_timeout_dropdown(lv_event_t * e){
    lv_obj_t * dropdown = lv_event_get_target(e);
    auto selected = lv_dropdown_get_selected(dropdown);
    global_config.screen_timeout = wake_timeout_options_values[selected];
    set_screen_timer_period();
    write_global_config();
}

static void rotate_screen_switch(lv_event_t* e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    global_config.rotate_screen = checked;
    global_config.screen_calibrated = false;
    write_global_config();
    ESP.restart();
}

static void on_during_print_switch(lv_event_t* e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    global_config.on_during_print = checked;
    check_if_screen_needs_to_be_disabled();
    write_global_config();
}

static void btn_ota_do_update(lv_event_t * e){
    set_ready_for_ota_update();
}

static void auto_ota_update_switch(lv_event_t* e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    global_config.auto_ota_update = checked;
    write_global_config();
}

static void multi_printer_switch(lv_event_t* e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    global_config.multi_printer_mode = checked;
    write_global_config();
    nav_buttons_setup(PANEL_SETTINGS);
}

const char* estimated_time_options = "Percentage\nInterpolated\nSlicer";

static void estimated_time_dropdown(lv_event_t * e){
    lv_obj_t * dropdown = lv_event_get_target(e);
    get_current_printer_config()->remaining_time_calc_mode = lv_dropdown_get_selected(dropdown);
    write_global_config();
}

void settings_panel_init(lv_obj_t* panel){
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);
    lv_layout_flex_column(panel);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);

    if (global_config.multi_printer_mode)
    {
        lv_obj_t * label = lv_label_create(panel);
        lv_label_set_text(label, "Printer Specific Settings");
    }

    lv_create_custom_menu_dropdown("Estimated Time", panel, estimated_time_dropdown, estimated_time_options, get_current_printer_config()->remaining_time_calc_mode);
    lv_create_custom_menu_dropdown("Theme", panel, theme_dropdown, "Blue\nGreen\nGrey\nYellow\nOrange\nRed\nPurple", get_current_printer_config()->color_scheme);

#ifndef CYD_SCREEN_DISABLE_INVERT_COLORS
    lv_create_custom_menu_switch("Invert Colors", panel, invert_color_switch, get_current_printer_config()->invert_colors);
#endif // CYD_SCREEN_DISABLE_INVERT_COLORS

    lv_create_custom_menu_switch("Light Mode", panel, light_mode_switch, get_current_printer_config()->light_mode);
    lv_create_custom_menu_button("Configure IP", panel, reset_ip_click, "Restart");

    if (global_config.multi_printer_mode)
    {
        lv_obj_t * label = lv_label_create(panel);
        lv_label_set_text(label, "\nGlobal Settings");
    }

#ifndef CYD_SCREEN_DISABLE_TIMEOUT
    lv_create_custom_menu_switch("Screen On During Print", panel, on_during_print_switch, global_config.on_during_print);
#endif

    int brightness_settings_index = 0;
    for (int i = 0; i < SIZEOF(brightness_options_values); i++){
        if (brightness_options_values[i] == global_config.brightness){
            brightness_settings_index = i;
            break;
        }
    }

    lv_create_custom_menu_dropdown("Brightness", panel, brightness_dropdown, brightness_options, brightness_settings_index);

#ifndef CYD_SCREEN_DISABLE_TIMEOUT
    int wake_timeout_settings_index = 0;
    for (int i = 0; i < SIZEOF(wake_timeout_options_values); i++){
        if (wake_timeout_options_values[i] == global_config.screen_timeout){
            wake_timeout_settings_index = i;
            break;
        }
    }

    lv_create_custom_menu_dropdown("Wake Timeout", panel, wake_timeout_dropdown, wake_timeout_options, wake_timeout_settings_index);
#endif

    lv_create_custom_menu_switch("Rotate Screen", panel, rotate_screen_switch, global_config.rotate_screen);
    lv_create_custom_menu_switch("Multi Printer Mode", panel, multi_printer_switch, global_config.multi_printer_mode);
    lv_create_custom_menu_switch("Auto Update", panel, auto_ota_update_switch, global_config.auto_ota_update);
    lv_create_custom_menu_label("Version", panel, REPO_VERSION "  ");

    if (ota_has_update()){
        lv_obj_t *btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, btn_ota_do_update, LV_EVENT_CLICKED, NULL);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "Update to %s", ota_new_version_name().c_str());
        lv_obj_center(label);

        lv_create_custom_menu_entry("Device", btn, panel);
    }
    else {
        lv_create_custom_menu_label("Device", panel, ARDUINO_BOARD "  ");
    }

    #ifndef CYD_SCREEN_DISABLE_TOUCH_CALIBRATION
        lv_create_custom_menu_button("Calibrate Touch", panel, reset_calibration_click, "Restart");
    #endif // CYD_SCREEN_DISABLE_TOUCH_CALIBRATION

    lv_create_custom_menu_button("Configure WiFi", panel, reset_wifi_click, "Restart");
}