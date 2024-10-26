#include "../macros.h"
#include "panel.h"
#include "../nav_buttons.h"
#include "../../core/data_setup.h"
#include "../../conf/global_config.h"
#include "../ui_utils.h"
#include <HardwareSerial.h>

static void btn_goto_settings(lv_event_t * e){
    nav_buttons_setup(PANEL_SETTINGS);
}

void macros_panel_init(lv_obj_t* panel) {
    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_add_event_cb(btn, btn_goto_settings, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, CYD_SCREEN_GAP_PX);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS " Screen Settings");
    lv_obj_center(label);

    lv_obj_t * root_panel = lv_create_empty_panel(panel);
    lv_obj_set_scrollbar_mode(root_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(root_panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX - CYD_SCREEN_MIN_BUTTON_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2); 
    lv_obj_align(root_panel, LV_ALIGN_TOP_MID, 0, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX + CYD_SCREEN_GAP_PX * 2);
    lv_layout_flex_column(root_panel);

    int power_count = macros_add_power_devices_to_panel(root_panel, get_current_printer());
    int macros_count = macros_add_macros_to_panel(root_panel, get_current_printer());

    if (macros_count <= 0){
        label = lv_label_create(root_panel);
        lv_label_set_text(label, "No macros found.\nMacros with the description\n\"CYD_SCREEN_MACRO\"\nwill show up here.");

        if (power_count <= 0){
            lv_layout_flex_column(root_panel, LV_FLEX_ALIGN_CENTER);
        }

        return;
    }
}