#include "lvgl.h"
#include "panel.h"
#include "../../core/screen_driver.h"
#include "../../conf/global_config.h"

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
    WriteGlobalConfig();
    set_color_scheme();
}

void settings_panel_init(lv_obj_t* panel){
    auto panel_width = TFT_HEIGHT - 40;

    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 10, 5);
    lv_obj_add_event_cb(btn, reset_wifi_click, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn, panel_width / 2 - 15, 30);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "WiFi Setup");
    lv_obj_center(label);

    btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_add_event_cb(btn, reset_calibration_click, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn, panel_width / 2 - 15, 30);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Touch Cal");
    lv_obj_center(label);


    lv_obj_t * toggle = lv_switch_create(panel);
    lv_obj_align(toggle, LV_ALIGN_TOP_RIGHT, -14, 57);
    lv_obj_add_event_cb(toggle, invert_color_switch, LV_EVENT_VALUE_CHANGED, NULL);

    if (global_config.invertColors)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    label = lv_label_create(panel);
    lv_label_set_text(label, "Invert Colors");
    lv_obj_align(label, LV_ALIGN_TOP_RIGHT, -10, 40);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);

    toggle = lv_switch_create(panel);
    lv_obj_align(toggle, LV_ALIGN_TOP_LEFT, 13, 57);
    lv_obj_add_event_cb(toggle, light_mode_switch, LV_EVENT_VALUE_CHANGED, NULL);

    if (global_config.lightMode)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    label = lv_label_create(panel);
    lv_label_set_text(label, "Light Mode");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);

    lv_obj_t * dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(dropdown, "Blue\nGreen\nGrey\nYellow\nOrange\nRed\nPurple");
    lv_dropdown_set_selected(dropdown, global_config.color_scheme);
    lv_obj_align(dropdown, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_add_event_cb(dropdown, theme_dropdown, LV_EVENT_VALUE_CHANGED, NULL);

    label = lv_label_create(panel);
    lv_label_set_text(label, "Theme");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
}