#include "panel.h"
#include "../../core/data_setup.h"
#include "../ui_utils.h"
#include "../../core/printer_integration.hpp"

static void btn_click_restart(lv_event_t * e){
    get_current_printer()->execute_feature(PrinterFeatureRestart);
}

static void btn_click_firmware_restart(lv_event_t * e){
    get_current_printer()->execute_feature(PrinterFeatureFirmwareRestart);
}

static void btn_click_error_ignore(lv_event_t * e){
    get_current_printer()->execute_feature(PrinterFeatureIgnoreError);
}

static void btn_click_error_continue(lv_event_t * e){
    get_current_printer()->execute_feature(PrinterFeatureContinueError);
}

static void btn_click_error_retry(lv_event_t * e){
    get_current_printer()->execute_feature(PrinterFeatureRetryError);
}

static void set_state_message_text(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, get_current_printer_data()->state_message);
}

void create_button(const char* label, lv_event_cb_t on_click, lv_obj_t * root){
    lv_obj_t* btn = lv_btn_create(root);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_obj = lv_label_create(btn);
    lv_label_set_text(label_obj, label);
    lv_obj_center(label_obj);
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
    lv_obj_set_width(label, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_add_event_cb(label, set_state_message_text, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, label, NULL);
    
    lv_obj_t * button_row = lv_create_empty_panel(panel);
    lv_obj_set_size(button_row, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_layout_flex_row(button_row);

    if (get_current_printer_data()->error_screen_features & PrinterFeatureRestart)
    {
        create_button("Restart", btn_click_restart, button_row);
    }

    if (get_current_printer_data()->error_screen_features & PrinterFeatureFirmwareRestart)
    {
        create_button("FW Restart", btn_click_firmware_restart, button_row);
    }

    if (get_current_printer_data()->error_screen_features & PrinterFeatureIgnoreError)
    {
        create_button("Ignore", btn_click_error_ignore, button_row);
    }

    if (get_current_printer_data()->error_screen_features & PrinterFeatureContinueError)
    {
        create_button("Continue", btn_click_error_continue, button_row);
    }

    if (get_current_printer_data()->error_screen_features & PrinterFeatureRetryError)
    {
        create_button("Retry", btn_click_error_retry, button_row);
    }
}