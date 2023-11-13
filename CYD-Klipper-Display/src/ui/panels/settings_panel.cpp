#include "lvgl.h"
#include "panel.h"
#include "../../core/screen_driver.h"
#include "../../conf/global_config.h"
#include "../main_ui.h"

static void invert_color_switch(lv_event_t * e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    global_config.invertColors = checked;
    WriteGlobalConfig();
    set_invert_display();
}

static void reset_calibration_click(lv_event_t * e){
    global_config.screenCalibrated = false;
    WriteGlobalConfig();
    ESP.restart();
}

static void reset_wifi_click(lv_event_t * e){
    global_config.wifiConfigured = false;
    global_config.ipConfigured = false;
    WriteGlobalConfig();
    ESP.restart();
}

static void light_mode_switch(lv_event_t * e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    global_config.lightMode = checked;
    WriteGlobalConfig();
    set_color_scheme();
}

static void theme_dropdown(lv_event_t * e){
    lv_obj_t * dropdown = lv_event_get_target(e);
    auto selected = lv_dropdown_get_selected(dropdown);
    global_config.color_scheme = selected;
    set_color_scheme();
    WriteGlobalConfig();
}

const char* brightness_options = "100%\n75%\n50%\n25%";
const char  brightness_options_values[] = { 255, 192, 128, 64 };

static void brightness_dropdown(lv_event_t * e){
    lv_obj_t * dropdown = lv_event_get_target(e);
    auto selected = lv_dropdown_get_selected(dropdown);
    global_config.brightness = brightness_options_values[selected];
    set_screen_brightness();
    WriteGlobalConfig();
}

const char* wake_timeout_options = "1m\n2m\n5m\n10m\n15m\n30m\n1h\n2h\n4h";
const char  wake_timeout_options_values[] = { 1, 2, 5, 10, 15, 30, 60, 120, 240 };

static void wake_timeout_dropdown(lv_event_t * e){
    lv_obj_t * dropdown = lv_event_get_target(e);
    auto selected = lv_dropdown_get_selected(dropdown);
    global_config.screenTimeout = wake_timeout_options_values[selected];
    set_screen_timer_period();
    WriteGlobalConfig();
}

static void rotate_screen_switch(lv_event_t* e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    global_config.rotateScreen = checked;
    global_config.screenCalibrated = false;
    WriteGlobalConfig();
    ESP.restart();
}

static void on_during_print_switch(lv_event_t* e){
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    global_config.onDuringPrint = checked;
    check_if_screen_needs_to_be_disabled();
    WriteGlobalConfig();
}

int y_offset = 0;
const int y_element_size = 50;
const int y_seperator_size = 1;
const int y_seperator_x_padding = 50;
const int panel_width = TFT_HEIGHT - 40;
const int y_element_x_padding = 30;
const static lv_point_t line_points[] = { {0, 0}, {panel_width - y_seperator_x_padding, 0} };

void create_settings_widget(const char* label_text, lv_obj_t* object, lv_obj_t* root_panel){
    lv_obj_t * panel = lv_obj_create(root_panel);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, y_offset);
    lv_obj_set_size(panel, panel_width - y_element_x_padding, y_element_size);

    lv_obj_t * line = lv_line_create(panel);
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, y_seperator_size, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(line, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_set_parent(object, panel);
    lv_obj_align(object, LV_ALIGN_RIGHT_MID, 0, 0);
    y_offset += y_element_size;
}

void settings_panel_init(lv_obj_t* panel){
    y_offset = 0;

    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_add_event_cb(btn, reset_wifi_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "Restart");
    lv_obj_center(label);

    create_settings_widget("Configure WiFi", btn, panel);

    btn = lv_btn_create(panel);
    lv_obj_add_event_cb(btn, reset_calibration_click, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Restart");
    lv_obj_center(label);

    create_settings_widget("Calibrate Touch", btn, panel);

    lv_obj_t * toggle = lv_switch_create(panel);
    lv_obj_add_event_cb(toggle, invert_color_switch, LV_EVENT_VALUE_CHANGED, NULL);

    if (global_config.invertColors)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    create_settings_widget("Invert Colors", toggle, panel);


    toggle = lv_switch_create(panel);
    lv_obj_add_event_cb(toggle, light_mode_switch, LV_EVENT_VALUE_CHANGED, NULL);

    if (global_config.lightMode)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    create_settings_widget("Light Mode", toggle, panel);

    lv_obj_t * dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(dropdown, "Blue\nGreen\nGrey\nYellow\nOrange\nRed\nPurple");
    lv_dropdown_set_selected(dropdown, global_config.color_scheme);
    lv_obj_add_event_cb(dropdown, theme_dropdown, LV_EVENT_VALUE_CHANGED, NULL);

    create_settings_widget("Theme", dropdown, panel);

    dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(dropdown, brightness_options);
    lv_obj_add_event_cb(dropdown, brightness_dropdown, LV_EVENT_VALUE_CHANGED, NULL);

    for (int i = 0; i < SIZEOF(brightness_options_values); i++){
        if (brightness_options_values[i] == global_config.brightness){
            lv_dropdown_set_selected(dropdown, i);
            break;
        }
    }

    create_settings_widget("Brightness", dropdown, panel);

    dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(dropdown, wake_timeout_options);
    lv_obj_add_event_cb(dropdown, wake_timeout_dropdown, LV_EVENT_VALUE_CHANGED, NULL);

    for (int i = 0; i < SIZEOF(wake_timeout_options_values); i++){
        if (wake_timeout_options_values[i] == global_config.screenTimeout){
            lv_dropdown_set_selected(dropdown, i);
            break;
        }
    }

    create_settings_widget("Wake Timeout", dropdown, panel);

    toggle = lv_switch_create(panel);
    lv_obj_add_event_cb(toggle, rotate_screen_switch, LV_EVENT_VALUE_CHANGED, NULL);

    if (global_config.rotateScreen)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);
    
    create_settings_widget("Rotate Screen", toggle, panel);

    toggle = lv_switch_create(panel);
    lv_obj_add_event_cb(toggle, on_during_print_switch, LV_EVENT_VALUE_CHANGED, NULL);

    if (global_config.onDuringPrint)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    create_settings_widget("Screen On During Print", toggle, panel);
}