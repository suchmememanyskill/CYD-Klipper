#include "lvgl.h"
#include "panel.h"
#include "../nav_buttons.h"
#include "../../core/data_setup.h"
#include "../../core/macros_query.h"
#include <HardwareSerial.h>

int y_offset_macros = 40;
const int y_element_size = 50;
const int y_seperator_size = 1;
const int y_seperator_x_padding = 50;
const int panel_width = TFT_HEIGHT - 40;
const int y_element_x_padding = 30;
const static lv_point_t line_points[] = { {0, 0}, {panel_width - y_seperator_x_padding, 0} };

static void btn_press(lv_event_t * e){
    lv_obj_t * btn = lv_event_get_target(e);
    const char* macro = (const char*)lv_event_get_user_data(e);
    Serial.printf("Macro: %s\n", macro);
    send_gcode(false, macro);
}

static void btn_goto_settings(lv_event_t * e){
    nav_buttons_setup(3);
}

void create_macro_widget(const char* macro, lv_obj_t* root_panel){
    lv_obj_t * panel = lv_obj_create(root_panel);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, y_offset_macros);
    lv_obj_set_size(panel, panel_width - y_element_x_padding, y_element_size);

    lv_obj_t * line = lv_line_create(panel);
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, y_seperator_size, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(line, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, macro);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(btn, btn_press, LV_EVENT_CLICKED, (void*)macro);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Run");
    lv_obj_center(label);

    y_offset_macros += y_element_size;
}

void macros_panel_init(lv_obj_t* panel) {
    y_offset_macros = 40;

    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_add_event_cb(btn, btn_goto_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn, TFT_HEIGHT - 40 - 20, 30);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS " Screen Settings");
    lv_obj_center(label);

    MACROSQUERY query = macros_query();
    if (query.count == 0){
        label = lv_label_create(panel);
        lv_label_set_text(label, "No macros found.\nMacros with the description\n\"CYD_SCREEN_MACRO\"\nwill show up here.");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    for (int i = 0; i < query.count; i++){
        create_macro_widget(query.macros[i], panel);
    }
}