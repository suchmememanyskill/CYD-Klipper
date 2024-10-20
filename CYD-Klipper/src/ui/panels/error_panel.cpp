#include "panel.h"
#include "../../core/data_setup.h"
#include "../ui_utils.h"

static void btn_click_restart(lv_event_t * e){
    send_gcode(false, "RESTART");
}

static void btn_click_firmware_restart(lv_event_t * e){
    send_gcode(false, "FIRMWARE_RESTART");
}

void error_panel_init(lv_obj_t* panel) 
{
    lv_layout_flex_column(panel, LV_FLEX_ALIGN_SPACE_BETWEEN);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);

    lv_obj_t * label;
    label = lv_label_create(panel);
    lv_label_set_text(label, LV_SYMBOL_WARNING " Printer is not ready");

    lv_obj_t * panel_with_text = lv_create_empty_panel(panel);
    lv_layout_flex_column(panel_with_text, LV_FLEX_ALIGN_START);
    lv_obj_set_flex_grow(panel_with_text, 1);
    lv_obj_set_width(panel_with_text, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);

    label = lv_label_create(panel_with_text);
    lv_label_set_text(label, printer.state_message);
    lv_obj_set_width(label, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    
    lv_obj_t * button_row = lv_create_empty_panel(panel);
    lv_obj_set_size(button_row, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_layout_flex_row(button_row);

    lv_obj_t * btn = lv_btn_create(button_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, btn_click_restart, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Restart");
    lv_obj_center(label);

    btn = lv_btn_create(button_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, btn_click_firmware_restart, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, "FW Restart");
    lv_obj_center(label);
}